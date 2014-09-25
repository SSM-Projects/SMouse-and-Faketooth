#include "driver.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SPM_ConfigureContinuousReader)
#endif



NTSTATUS
SPM_ConfigureContinuousReader(
_In_ WDFDEVICE Device)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devContext;
	WDF_USB_CONTINUOUS_READER_CONFIG readerConfig;

	PAGED_CODE();

	devContext = GetDeviceContext(Device);



	//WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, SPM_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize);

#ifdef BULK
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, BulkReadPipe_MaximumPacketSize : %d\n", devContext->BulkReadPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, SPM_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize * sizeof(BYTE));
#endif
#ifdef INTERRUPT
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, InterruptPipe_MaximumPacketSize : %d\n", devContext->InterruptPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, SPM_EvtReadComplete, devContext, devContext->InterruptPipe_MaximumPacketSize * sizeof(BYTE));
#endif

	readerConfig.EvtUsbTargetPipeReadersFailed = (PFN_WDF_USB_READERS_FAILED)SPM_EvtReadFailed;
	readerConfig.NumPendingReads = 2; // this is by default = 0;

#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WDF_USB_CONTINUOUS_READER_CONFIG Params \n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - Size: %d, \n", readerConfig.Size);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - TransferLength: %d, \n", readerConfig.TransferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - HeaderLength: %d, \n", readerConfig.HeaderLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - TrailerLength: %d, \n", readerConfig.TrailerLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - NumPendingReads: %d, \n", readerConfig.NumPendingReads);
#endif

#ifdef BULK
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->BulkINPipe, &readerConfig);
#endif
#ifdef INTERRUPT
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->InterruptINPipe, &readerConfig);
#endif

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfUsbTargetPipeConfigContinuousReader failed 0x%x\n", ntStatus);
#endif
		goto Exit;
	}

#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT EXIT\n");
#endif

Exit:
	return ntStatus;
}

/*
* Continuous Reader - Complete
* SPM_ConfigureContinuousReader에서 Callback 등록
*/
VOID
SPM_EvtReadComplete(
__in  WDFUSBPIPE Pipe,
__in  WDFMEMORY Buffer,
__in  size_t NumBytesTransferred,
__in  WDFCONTEXT Context)
{
	PDEVICE_EXTENSION  devContext;
	PVOID  requestBuffer;
	//ULONG InputDataConsumed = 0;
	//KIRQL OldIrql;

#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - SPM_EvtReadComplete \n");
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
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - ReadBuffer: %s \n", requestBuffer);
#endif

		SPM_MouseReport(devContext, requestBuffer);
	}


	return;
}

/*
* Continuous Reader - Failed
* SPM_ConfigureContinuousReader에서 Callback 등록
*/
BOOLEAN
SPM_EvtReadFailed(
WDFUSBPIPE Pipe,
NTSTATUS ntStatus,
USBD_STATUS UsbdStatus)
{
	UNREFERENCED_PARAMETER(Pipe);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - Continuous Readers Failed : NTSTATUS 0x%x, UsbdStatus 0x%x\n", ntStatus, UsbdStatus);
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
SPM_MouseReport(
PDEVICE_EXTENSION devContext,
PVOID requestBuffer)
{
	PHID_MOUSE_DATA MouseData;
	NTSTATUS ntStatus;
	WDFREQUEST request;

	UNREFERENCED_PARAMETER(requestBuffer);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - SPM_MouseReport\n");
#endif

	ntStatus = WdfIoQueueRetrieveNextRequest(devContext->InterruptMsgQueue, &request);
	if (!NT_SUCCESS(ntStatus)){
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfIoQueueRetrieveNextRequest failed with status: 0x%x\n", ntStatus);
#endif
		return;
	}

	ntStatus = WdfRequestRetrieveOutputBuffer(request, sizeof(HID_MOUSE_DATA), &MouseData, NULL);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfRequestRetrieveOutputBuffer failed with status: 0x%x\n", ntStatus);
#endif
		WdfRequestComplete(request, ntStatus);
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	struct smouse {
		int btn;
		int wheel;
		int moveX;
		int moveY;
	};

	struct smouse *values = (struct smouse *)requestBuffer;

	static UCHAR btn = 0;
	static USHORT curX = 20000;
	static USHORT curY = 15000;

	if (values->btn != 0)
		btn = (UCHAR)values->btn;

	values->moveX = (INT)curX - values->moveX;
	values->moveY = (INT)curY + values->moveY;

	//curX -= (SHORT)values->moveX;
	//curY += (SHORT)values->moveY;

	if (values->moveX < 0)
		curX = 0;
	else if (values->moveX < 32767)
		curX = (USHORT)values->moveX;
	else
		curX = 32766;

	if (values->moveY < 0)
		curY = 0;
	else if (values->moveY < 32767)
		curY = (USHORT)values->moveY;
	else
		curY = 32766;


	MouseData->ID = REPORTID_MOUSE;
	MouseData->buttons = btn;
	MouseData->lastX = curX;
	MouseData->lastY = curY;
	MouseData->wheel = (USHORT)values->wheel;

	WdfRequestCompleteWithInformation(request, ntStatus, sizeof(HID_MOUSE_DATA));

	if (btn == 4 || btn == 5)
		btn = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	*  buttons
	*    0 : 이동
	*    1 : Left_Down
	*    2 : Right_Down
	*    3 :
	*    4 : Left_Up
	*    5 : RightUp
	*
	*  Wheel
	*    0 : 멈춤
	*    + : Up
	*    - : Down
	*/

	/*
	MouseData->ID = REPORTID_MOUSE;
	MouseData->buttons = 0;
	MouseData->lastX = 10000;
	MouseData->lastY = 10000;
	MouseData->wheel = 0;

	WdfRequestCompleteWithInformation(request, ntStatus, sizeof(HID_MOUSE_DATA));

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ntStatus = WdfIoQueueRetrieveNextRequest(devContext->InterruptMsgQueue, &request);
	if (!NT_SUCCESS(ntStatus)){
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfIoQueueRetrieveNextRequest failed with status: 0x%x\n", ntStatus);
	return;
	}

	ntStatus = WdfRequestRetrieveOutputBuffer(request, sizeof(HID_MOUSE_DATA), &MouseData, NULL);
	if (!NT_SUCCESS(ntStatus)) {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfRequestRetrieveOutputBuffer failed with status: 0x%x\n", ntStatus);
	WdfRequestComplete(request, ntStatus);
	return;
	}

	MouseData->ID = REPORTID_MOUSE;
	MouseData->buttons = 0;
	MouseData->lastX = 10000;
	MouseData->lastY = 10000;
	MouseData->wheel = 0;

	WdfRequestCompleteWithInformation(request, ntStatus, sizeof(HID_MOUSE_DATA));
	*/

}
