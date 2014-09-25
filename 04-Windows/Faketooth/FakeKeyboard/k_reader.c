#include "k_driver.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FakeKeyboard_ConfigureContinuousReader)
#endif



NTSTATUS
FakeKeyboard_ConfigureContinuousReader(
_In_ WDFDEVICE Device)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devContext;
	WDF_USB_CONTINUOUS_READER_CONFIG readerConfig;

	PAGED_CODE();

	devContext = GetDeviceContext(Device);



	//WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeKeyboard_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize);

#ifdef BULK
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, BulkReadPipe_MaximumPacketSize : %d\n", devContext->BulkReadPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeKeyboard_EvtReadComplete, devContext, BulkReadPipe_MaximumPacketSize * sizeof(BYTE));
#endif
#ifdef INTERRUPT
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT, InterruptPipe_MaximumPacketSize : %d\n", devContext->InterruptPipe_MaximumPacketSize);
#endif
	WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&readerConfig, FakeKeyboard_EvtReadComplete, devContext, devContext->InterruptPipe_MaximumPacketSize * sizeof(BYTE));
#endif

	readerConfig.EvtUsbTargetPipeReadersFailed = (PFN_WDF_USB_READERS_FAILED)FakeKeyboard_EvtReadFailed;
	readerConfig.NumPendingReads = 2; // this is by default = 0;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WDF_USB_CONTINUOUS_READER_CONFIG Params \n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - Size: %d, \n", readerConfig.Size);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - TransferLength: %d, \n", readerConfig.TransferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - HeaderLength: %d, \n", readerConfig.HeaderLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - TrailerLength: %d, \n", readerConfig.TrailerLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - NumPendingReads: %d, \n", readerConfig.NumPendingReads);
#endif

#ifdef BULK
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->BulkINPipe, &readerConfig);
#endif
#ifdef INTERRUPT
	ntStatus = WdfUsbTargetPipeConfigContinuousReader(devContext->InterruptINPipe, &readerConfig);
#endif

	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WdfUsbTargetPipeConfigContinuousReader failed 0x%x\n", ntStatus);
#endif
		goto Exit;
	}

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WDF_USB_CONTINUOUS_READER_CONFIG_INIT EXIT\n");
#endif

Exit:
	return ntStatus;
}

/*
* Continuous Reader - Complete
* FakeKeyboard_ConfigureContinuousReader에서 Callback 등록
*/
VOID
FakeKeyboard_EvtReadComplete(
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
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - FakeKeyboard_EvtReadComplete \n");
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
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - ReadBuffer: %s \n", requestBuffer);
#endif

		FakeKeyboard_KeyboardReport(devContext, requestBuffer);
	}


	return;
}

/*
* Continuous Reader - Failed
* FakeKeyboard_ConfigureContinuousReader에서 Callback 등록
*/
BOOLEAN
FakeKeyboard_EvtReadFailed(
WDFUSBPIPE Pipe,
NTSTATUS ntStatus,
USBD_STATUS UsbdStatus)
{
	UNREFERENCED_PARAMETER(Pipe);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - Continuous Readers Failed : NTSTATUS 0x%x, UsbdStatus 0x%x\n", ntStatus, UsbdStatus);
#else
	UNREFERENCED_PARAMETER(UsbdStatus);
	UNREFERENCED_PARAMETER(ntStatus);
#endif

	return TRUE;
}


/*
* 키보드 데이터 보내는 함수
*/

VOID
FakeKeyboard_KeyboardReport(
PDEVICE_EXTENSION devContext,
PVOID requestBuffer)
{
	PHID_KEYBOARD_DATA KeyboardData;
	NTSTATUS ntStatus;
	WDFREQUEST request;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - FakeKeyboard_KeyboardReport\n");
#endif

	ntStatus = WdfIoQueueRetrieveNextRequest(devContext->ReadMsgQueue, &request);
	if (!NT_SUCCESS(ntStatus)){
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WdfIoQueueRetrieveNextRequest failed with status: 0x%x\n", ntStatus);
#endif
		return;
	}

	ntStatus = WdfRequestRetrieveOutputBuffer(request, sizeof(HID_KEYBOARD_DATA), &KeyboardData, NULL);
	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - WdfRequestRetrieveOutputBuffer failed with status: 0x%x\n", ntStatus);
#endif
		WdfRequestComplete(request, ntStatus);
		return;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	struct skey {
		INT keyCode;
		INT scanCode;
		INT action;		// 0 : Down, 1 : Up
	};

	struct skey *values = (struct skey *)requestBuffer;

	static UCHAR tmp_modifier = 0;

	// 받은 값을 키보드 값으로 변형 후 전달

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - KeyCode : %d, ScanCode : %d, Action : %d\n", values->keyCode, values->scanCode, values->action);
#endif

	
	/* modifier key */
	// (values->keyCode == 218)	// Han/Eng
	// (values->keyCode == 212)	// Hanja
	// R_Gui, R_Alt, R_Shift, R_Contorl, L_Gui, L_Alt, L_Shift, L_Contorl
	//   118    58      60       114     117     57      59      113        // keycode
	//  0x80   0x40    0x20     0x10     0x08    0x04     0x02    0x01
	if (values->keyCode == 58 && values->action == 0)		
		tmp_modifier = tmp_modifier | 0x40;
	else if (values->keyCode == 58 && values->action == 1)	
		tmp_modifier = tmp_modifier & 0xbf;
	else if (values->keyCode == 218 && values->action == 0)	// KOR/ENG
		tmp_modifier = tmp_modifier | 0x40;
	else if (values->keyCode == 218 && values->action == 1)	// KOR/ENG
		tmp_modifier = tmp_modifier & 0xbf;
	else if (values->keyCode == 60 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x20;
	else if (values->keyCode == 60 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xdf;
	else if (values->keyCode == 114 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x10;
	else if (values->keyCode == 114 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xef;
	else if (values->keyCode == 212 && values->action == 0)	// Hanja
		tmp_modifier = tmp_modifier | 0x10;
	else if (values->keyCode == 212 && values->action == 1)	// Hanja
		tmp_modifier = tmp_modifier & 0xef;
	else if (values->keyCode == 57 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x04;
	else if (values->keyCode == 57 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xfb;
	else if (values->keyCode == 59 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x02;
	else if (values->keyCode == 59 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xfd;
	else if (values->keyCode == 113 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x01;
	else if (values->keyCode == 113 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xfe;
	else if (values->keyCode == 117 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x08;
	else if (values->keyCode == 117 && values->action == 1)
		tmp_modifier = tmp_modifier & 0xf7;
	else if (values->keyCode == 118 && values->action == 0)
		tmp_modifier = tmp_modifier | 0x80;
	else if (values->keyCode == 118 && values->action == 1)
		tmp_modifier = tmp_modifier & 0x7f;

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Keyboard Driver - tmp_modifier : %d\n", tmp_modifier);
#endif

	KeyboardData->ID = REPORTID_KEYBOARD;
	KeyboardData->modifier = tmp_modifier;
	KeyboardData->reserved = 0;
	KeyboardData->key[1] = 0x00;
	KeyboardData->key[2] = 0x00;
	KeyboardData->key[3] = 0x00;
	KeyboardData->key[4] = 0x00;
	KeyboardData->key[5] = 0x00;


	if (values->action == 0)
	{
		if (values->keyCode >= 29 && values->keyCode <= 54)		// a ~ z
			KeyboardData->key[0] = (UCHAR)values->keyCode - 25;
		else if (values->scanCode >= 2 && values->scanCode <= 11)	// 1 ~ 0
			KeyboardData->key[0] = (UCHAR)values->scanCode + 28;
		else if (values->keyCode >= 131 && values->keyCode <= 142)	// f1 ~ f12		
			KeyboardData->key[0] = (UCHAR)values->keyCode - 73;
		else if (values->keyCode == 62)		KeyboardData->key[0] = 44;	// space bar
		else if (values->keyCode == 66)		KeyboardData->key[0] = 40;	// enter
		else if (values->keyCode == 67)		KeyboardData->key[0] = 42;	// back space		
		else if (values->keyCode == 111)	KeyboardData->key[0] = 41;	// ESC		
		else if (values->keyCode == 68)		KeyboardData->key[0] = 53;	// ` or ~       	
		else if (values->keyCode == 61)		KeyboardData->key[0] = 43;	// TAB
		else if (values->keyCode == 115)	KeyboardData->key[0] = 57;	// Caps Lock
		else if (values->keyCode == 69)		KeyboardData->key[0] = 45;	// - or _
		else if (values->keyCode == 70)		KeyboardData->key[0] = 46;	// = or +
		else if (values->keyCode == 71)		KeyboardData->key[0] = 47;	// [ or {
		else if (values->keyCode == 72)		KeyboardData->key[0] = 48;	// ] or }
		else if (values->keyCode == 73)		KeyboardData->key[0] = 49;	// \ or |
		else if (values->keyCode == 74)		KeyboardData->key[0] = 51;	// ; or :
		else if (values->keyCode == 75)		KeyboardData->key[0] = 52;	// ' or "
		else if (values->keyCode == 55)		KeyboardData->key[0] = 54;	// , or <
		else if (values->keyCode == 56)		KeyboardData->key[0] = 55;	// . or >
		else if (values->keyCode == 76)		KeyboardData->key[0] = 56;	// / or ?
		else if (values->keyCode == 22)		KeyboardData->key[0] = 79;	// Right
		else if (values->keyCode == 21)		KeyboardData->key[0] = 80;	// Left
		else if (values->keyCode == 20)		KeyboardData->key[0] = 81;	// Down
		else if (values->keyCode == 19)		KeyboardData->key[0] = 82;	// Up
		else if (values->keyCode == 120)	KeyboardData->key[0] = 70;	// Print Screen
		else if (values->keyCode == 122)	KeyboardData->key[0] = 74;	// Home
		else if (values->keyCode == 123)	KeyboardData->key[0] = 77;	// End
		else if (values->keyCode == 92)		KeyboardData->key[0] = 75;	// Page Up
		else if (values->keyCode == 93)		KeyboardData->key[0] = 78;	// Page Down
		else if (values->keyCode == 112)	KeyboardData->key[0] = 76;	// Delete Forward	
		else if (values->keyCode == 82)		KeyboardData->key[0] = 101;	// Right_Click

		//else if (values->keyCode == 164)	KeyboardData->key[0] = 127;	// Mute
		//else if (values->keyCode == 25)		KeyboardData->key[0] = 129;	// Volume Down
		//else if (values->keyCode == 24)		KeyboardData->key[0] = 128;	// Volume Up
		else
			KeyboardData->key[0] = 0;
		
	}
	else
	{
		KeyboardData->key[0] = 0;
	}
	
	WdfRequestCompleteWithInformation(request, ntStatus, sizeof(HID_KEYBOARD_DATA));



	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////


}
