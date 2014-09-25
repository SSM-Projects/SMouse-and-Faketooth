#include "m_driver.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FakeMouse_ConfigureContinuousReader)
#endif



NTSTATUS
FakeMouse_ConfigureContinuousReader(
	_In_ WDFDEVICE Device)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devContext;
	WDF_USB_CONTINUOUS_READER_CONFIG readerConfig;

	PAGED_CODE();

	devContext = GetDeviceContext(Device);



	//WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeMouse_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize);

#ifdef BULK
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, BulkReadPipe_MaximumPacketSize : %d\n", devContext->BulkReadPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeMouse_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize * sizeof(BYTE));
#endif
#ifdef INTERRUPT
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, InterruptPipe_MaximumPacketSize : %d\n", devContext->InterruptPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeMouse_EvtReadComplete, devContext, devContext->InterruptPipe_MaximumPacketSize * sizeof(BYTE));
#endif

	readerConfig.EvtUsbTargetPipeReadersFailed = (PFN_WDF_USB_READERS_FAILED)FakeMouse_EvtReadFailed;
	readerConfig.NumPendingReads = 2; // this is by default = 0;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WDF_USB_CONTINUOUS_READER_CONFIG Params \n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Size: %d, \n", readerConfig.Size);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - TransferLength: %d, \n", readerConfig.TransferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - HeaderLength: %d, \n", readerConfig.HeaderLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - TrailerLength: %d, \n", readerConfig.TrailerLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - NumPendingReads: %d, \n", readerConfig.NumPendingReads);
#endif

#ifdef BULK
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->BulkINPipe, &readerConfig);
#endif
#ifdef INTERRUPT
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->InterruptINPipe, &readerConfig);
#endif

	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfUsbTargetPipeConfigContinuousReader failed 0x%x\n", ntStatus);
#endif
		goto Exit;
	}

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT EXIT\n");
#endif

Exit:
	return ntStatus;
}

/*
* Continuous Reader - Complete
* FakeMouse_ConfigureContinuousReader에서 Callback 등록
*/
VOID
FakeMouse_EvtReadComplete(
	__in  WDFUSBPIPE Pipe,
	__in  WDFMEMORY Buffer,
	__in  size_t NumBytesTransferred,
	__in  WDFCONTEXT Context)
{
	PDEVICE_EXTENSION  devContext;
	PVOID  requestBuffer;
	//ULONG InputDataConsumed = 0;
	//KIRQL OldIrql;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - FakeMouse_EvtReadComplete \n");
#endif

	devContext = (PDEVICE_EXTENSION)Context;


	if (NumBytesTransferred == 0)
	{
		return;
	}

	requestBuffer = WdfMemoryGetBuffer(Buffer, NULL);

#ifdef BULK
	if (Pipe == devContext->BulkINPipe)
#endif 
#ifdef INTERRUPT
	if (Pipe == devContext->InterruptINPipe)
#endif
	{
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - ReadBuffer: %s \n", requestBuffer);
#endif

		FakeMouse_MouseReport(devContext, requestBuffer);
	}


	return;
}

/*
* Continuous Reader - Failed
* FakeMouse_ConfigureContinuousReader에서 Callback 등록
*/
BOOLEAN
FakeMouse_EvtReadFailed(
	WDFUSBPIPE Pipe,
	NTSTATUS ntStatus,
	USBD_STATUS UsbdStatus)
{
	UNREFERENCED_PARAMETER(Pipe);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Continuous Readers Failed : NTSTATUS 0x%x, UsbdStatus 0x%x\n", ntStatus, UsbdStatus);
#else
	UNREFERENCED_PARAMETER(UsbdStatus);
	UNREFERENCED_PARAMETER(ntStatus);
#endif

	return TRUE;
}


/*
* 마우스 데이터 보내는 함수
*/

VOID
FakeMouse_MouseReport(
	PDEVICE_EXTENSION devContext,
	PVOID requestBuffer)
{
	PHID_MOUSE_DATA MouseData;
	NTSTATUS ntStatus;
	WDFREQUEST request;

	ntStatus = WdfIoQueueRetrieveNextRequest(devContext->InterruptMsgQueue, &request);
	if (!NT_SUCCESS(ntStatus)){
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfIoQueueRetrieveNextRequest failed with status: 0x%x\n", ntStatus);
#endif
		return;
	}

	ntStatus = WdfRequestRetrieveOutputBuffer(request, sizeof(HID_MOUSE_DATA), &MouseData, NULL);
	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfRequestRetrieveOutputBuffer failed with status: 0x%x\n", ntStatus);
#endif
		WdfRequestComplete(request, ntStatus);
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	struct smouse {
		INT deltaX;
		INT deltaY;
		INT Vscroll;
		INT Hscroll;
		INT btn;	// 001 : Left , 010 : Right , 100 : whell
		INT action;	// 0 : Down , 1 : Up , 7 : Move
	};

	struct smouse *values = (struct smouse *)requestBuffer;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - deltaX : %d, deltaY : %d, Vscroll : %d, Hscroll : %d, btn : %d, action : %d \n", 
		values->deltaX, values->deltaY, values->Vscroll, values->Hscroll, values->btn, values->action);
#endif

	static UCHAR btn = 0;
	USHORT deltaX, deltaY;

	if (values->btn == 1 && values->action == 0)
		btn = 1;
	else if (values->btn == 1 && values->action == 2)
		btn = 1;
	else if (btn == 1 && values->btn == 0 && values->action == 1)
		btn = 0;
	else if (values->btn == 2 && values->action == 0)
		btn = 2;
	else if (values->btn == 2 && values->action == 2)
		btn = 2;
	else if (btn == 2 && values->btn == 0 && values->action == 1)
		btn = 0;
	else if (values->btn == 3 && values->action == 0)
		btn = 3;
	else if (values->btn == 3 && values->action == 2)
		btn = 3;
	else if (btn == 3 && values->btn == 0 && values->action == 1)
		btn = 0;
	else if (values->btn == 4 && values->action == 0)
		btn = 4;
	else if (values->btn == 4 && values->action == 2)
		btn = 4;
	else if (btn == 6 && values->btn == 4 && values->action == 1)
		btn = 0;
	else
		btn = 0;


	//if ((values->deltaX >= -8) && (values->deltaX <= 8))
	//	deltaX = (USHORT)(values->deltaX);
	//else if ((values->deltaX >= -15) && (values->deltaX <= 15))
	//	deltaX = (USHORT)(values->deltaX / 2);
	//else
	//	deltaX = (USHORT)(values->deltaX / 3);

	//if ((values->deltaY >= -8) && (values->deltaY <= 8))
	//	deltaY = (USHORT)(values->deltaY);
	//else if ((values->deltaY >= -15) && (values->deltaY <= 15))
	//	deltaY = (USHORT)(values->deltaY / 2);
	//else
	//	deltaY = (USHORT)(values->deltaY / 3);

	deltaX = (USHORT)(values->deltaX);
	deltaY = (USHORT)(values->deltaY);

	MouseData->ID = REPORTID_MOUSE;
	MouseData->buttons = btn;
	MouseData->lastX = deltaX;
	MouseData->lastY = deltaY;
	MouseData->wheel = (USHORT)values->Vscroll;

	WdfRequestCompleteWithInformation(request, ntStatus, sizeof(HID_MOUSE_DATA));

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	*  buttons
	*    0 : 이동 | Up
	*    1 : Left_Down
	*    2 : Right_Down
	*    3 : L&R_Down
	*	 4 : Whell_Down

	*
	*  Wheel
	*    0 : 멈춤
	*    + : Up
	*    - : Down
	*/

	

}
