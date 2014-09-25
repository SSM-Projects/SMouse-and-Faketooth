#include "driver.h"
#include "queue.h"


#ifdef ALLOC_PRAGMA
//#pragma alloc_text( PAGE, HidFx2SetFeature)
//#pragma alloc_text( PAGE, HidFx2GetFeature)
//#pragma alloc_text( PAGE, SendVendorCommand)
//#pragma alloc_text( PAGE, GetVendorData)
#endif



NTSTATUS
SPM_QueueInitialize(
_In_ WDFDEVICE Device)
{
	NTSTATUS				ntStatus;
	PDEVICE_EXTENSION		devContext;
	WDF_IO_QUEUE_CONFIG		ioQueueConfig;
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_QueueInitialize\n");
#endif

	devContext = GetDeviceContext(Device);

	// 디폴트 큐 생성
	//WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

	//ioQueueConfig.EvtIoDeviceControl = SPM_EvtIoDeviceControl;
	//ioQueueConfig.EvtIoDefault = SPM_EvtIoDefault;

	//ntStatus = WdfIoQueueCreate(Device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &devContext->IoDefaultQueue);	// default Queue

	//if (!NT_SUCCESS(ntStatus)) {
	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Default Queue Create failed 0x%x\n", ntStatus);
	//	return ntStatus;
	//}

	// Mouse 기능을 위한 큐 생성
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);
	ioQueueConfig.EvtIoDefault = SPM_EvtIoDefault;
	ioQueueConfig.EvtIoInternalDeviceControl = SPM_EvtIoInternalDeviceControl;

	ntStatus = WdfIoQueueCreate(Device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &devContext->IoMouseQueue);	// Mouse Queue

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Mouse Queue Create failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);
	ioQueueConfig.PowerManaged = WdfFalse;

	ntStatus = WdfIoQueueCreate(Device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &devContext->InterruptMsgQueue);	// PowerManaged Queue

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Interrupt Msg Queue Create failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	// 읽기 요청에 대한 순차 큐 생성
	WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);

	ioQueueConfig.EvtIoRead = SPM_EvtIoRead;
	ioQueueConfig.EvtIoStop = SPM_EvtIoStop;

	ntStatus = WdfIoQueueCreate(Device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &devContext->IoReadQueue);	// read Queue

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Read Queue Create failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	ntStatus = WdfDeviceConfigureRequestDispatching(Device, devContext->IoReadQueue, WdfRequestTypeRead);
	if (!NT_SUCCESS(ntStatus)) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Read Queue setting failed 0x%x\n", ntStatus);
		return ntStatus;
	}

	// 쓰기 요청에 대한 순차 큐 생성
	WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchSequential);

	ioQueueConfig.EvtIoWrite = SPM_EvtIoWrite;
	ioQueueConfig.EvtIoStop = SPM_EvtIoStop;

	ntStatus = WdfIoQueueCreate(Device, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &devContext->IoWriteQueue);	// write Queue

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - write Queue Create failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	ntStatus = WdfDeviceConfigureRequestDispatching(Device, devContext->IoWriteQueue, WdfRequestTypeWrite);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - write Queue setting failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	return ntStatus;
}

VOID
SPM_EvtIoDefault(
_In_ WDFQUEUE Queue,
_In_ WDFREQUEST Request)
{
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_EvtIoDefault Queue 0x%p, Request 0x%p", Queue, Request);
#else
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
#endif

	return;
}


VOID
SPM_EvtIoStop(
_In_ WDFQUEUE Queue,
_In_ WDFREQUEST Request,
_In_ ULONG ActionFlags)
{
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_EvtIoStop, Queue 0x%p, Request 0x%p ActionFlags %d",
		Queue, Request, ActionFlags);
#else
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(ActionFlags);
#endif

	return;
}

VOID
SPM_EvtIoRead(
_In_ WDFQUEUE Queue,
_In_ WDFREQUEST Request,
_In_ size_t Length)
{
	NTSTATUS ntStatus;
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_EvtIoRead Start\n");
#endif
	// 유효성 검사 
	if (Length > TEST_BOARD_TRANSFER_BUFFER_SIZE) {
		ntStatus = STATUS_INVALID_PARAMETER;
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - STATUS_INVALID_PARAMETER failed 0x%x === \n", ntStatus);
#endif
		goto Exit;
	}

	// Get MDL for this request.
	PMDL request_mdl = NULL;
	ntStatus = WdfRequestRetrieveOutputWdmMdl(Request, &request_mdl);

	if (NT_SUCCESS(ntStatus)) {
		CommonBulkReadWrite(Queue,
			Request,
			request_mdl,
			Length,
			TRUE,
			0,
			FALSE);
	}

Exit:
	if (!NT_SUCCESS(ntStatus)) {
		WdfRequestCompleteWithInformation(Request, ntStatus, 0);
	}

	return;
}


VOID
SPM_EvtIoWrite(
_In_ WDFQUEUE Queue,
_In_ WDFREQUEST Request,
_In_ size_t Length)
{
	NTSTATUS ntStatus;
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_EvtIoWrite Start\n");
#endif
	// 유효성 검사 
	if (Length > TEST_BOARD_TRANSFER_BUFFER_SIZE) {
		ntStatus = STATUS_INVALID_PARAMETER;
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - STATUS_INVALID_PARAMETER failed 0x%x === \n", ntStatus);
#endif
		goto Exit;
	}

	// Get MDL for this request.
	PMDL request_mdl = NULL;
	ntStatus = WdfRequestRetrieveInputWdmMdl(Request, &request_mdl);

	if (NT_SUCCESS(ntStatus)) {
		ntStatus = CommonBulkReadWrite(Queue,
			Request,
			request_mdl,
			Length,
			FALSE,
			0,
			FALSE);
	}


Exit:
	if (!NT_SUCCESS(ntStatus)) {
		WdfRequestCompleteWithInformation(Request, ntStatus, 0);
	}

	return;
}



NTSTATUS CommonBulkReadWrite(
	WDFQUEUE Queue,
	WDFREQUEST Request,
	PMDL transfer_mdl,
	ULONG length,
	BOOLEAN is_read,
	ULONG time_out,
	BOOLEAN is_ioctl)
{
	PDEVICE_EXTENSION devContext;
	WDFUSBPIPE Pipe;
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - CommonBulkReadWrite Start\n");
#endif

	devContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
#ifdef BULK
	if (is_read)
		Pipe = devContext->BulkINPipe;
	else
		Pipe = devContext->BulkOUTPipe;
#endif
#ifdef INTERRUPT
	if (is_read)
		Pipe = devContext->InterruptINPipe;
	else
		Pipe = devContext->InterruptOUTPipe;
#endif

	// URB flag setting
	ULONG urb_flags = USBD_SHORT_TRANSFER_OK | (is_read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT);

	// length 계산
	ULONG stage_len = (length > devContext->BulkReadPipe_MaximumPacketSize) ? devContext->BulkReadPipe_MaximumPacketSize : length;

	// 전송에 쓰일 virtual address
	void* virtual_address = MmGetMdlVirtualAddress(transfer_mdl);

	// MDL 할당
	PMDL new_mdl = IoAllocateMdl(virtual_address, length, FALSE, FALSE, NULL);
	if (NULL == new_mdl) {
		WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// 할당한 MDL에 매핑
	IoBuildPartialMdl(transfer_mdl, new_mdl, virtual_address, stage_len);


	WDF_OBJECT_ATTRIBUTES mem_attrib;
	WDF_OBJECT_ATTRIBUTES_INIT(&mem_attrib);
	mem_attrib.ParentObject = Request;

	WDFMEMORY urb_mem = NULL;
	PURB urb = NULL;
	NTSTATUS ntStatus =
		WdfMemoryCreate(&mem_attrib,
		NonPagedPool,
		'ubAG',
		sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
		&urb_mem,
		&urb);

	if (!NT_SUCCESS(ntStatus)) {
		IoFreeMdl(new_mdl);
		WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// USB Pipe Handle
	USBD_PIPE_HANDLE usbd_pipe_hndl = WdfUsbTargetPipeWdmGetPipeHandle(Pipe);
	if (NULL == usbd_pipe_hndl) {
		IoFreeMdl(new_mdl);
		WdfRequestComplete(Request, STATUS_INTERNAL_ERROR);
		return STATUS_INTERNAL_ERROR;
	}

	// URB 초기화
	UsbBuildInterruptOrBulkTransferRequest(
		urb,
		sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
		usbd_pipe_hndl,
		NULL,
		new_mdl,
		stage_len,
		urb_flags,
		NULL);

	// request bulid
	ntStatus = WdfUsbTargetPipeFormatRequestForUrb(Pipe,
		Request,
		urb_mem,
		NULL);

	if (!NT_SUCCESS(ntStatus)) {
		IoFreeMdl(new_mdl);
		WdfRequestComplete(Request, ntStatus);
		return ntStatus;
	}


	devContext->urb_mem = urb_mem;
	devContext->transfer_mdl = transfer_mdl;
	devContext->mdl = new_mdl;
	devContext->length = length;
	devContext->transfer_size = stage_len;
	devContext->num_xfer = 0;
	devContext->virtual_address = virtual_address;
	devContext->is_read = is_read;
	devContext->initial_time_out = time_out;
	devContext->is_ioctl = is_ioctl;

	// 완료 루틴
	WdfRequestSetCompletionRoutine(Request,
		CommonReadWriteCompletionEntry,
		devContext);

	// send option, timeout
	WDF_REQUEST_SEND_OPTIONS send_options;
	if (0 != time_out) {
		WDF_REQUEST_SEND_OPTIONS_INIT(&send_options, WDF_REQUEST_SEND_OPTION_TIMEOUT);
		WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&send_options, WDF_REL_TIMEOUT_IN_MS(time_out));
	}


	// request 전송
	if (WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(Pipe),
		(0 == time_out) ? WDF_NO_SEND_OPTIONS : &send_options)) {
		return STATUS_SUCCESS;
	}

	ntStatus = WdfRequestGetStatus(Request);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - CommonBulkReadWrite: WdfRequestGetStatus (is_read = %u) failed: %08X\n", is_read, ntStatus);
#endif
	WdfRequestCompleteWithInformation(Request, ntStatus, 0);

	return ntStatus;
}

VOID
CommonReadWriteCompletionEntry(
_In_ WDFREQUEST Request,
_In_ WDFIOTARGET Target,
PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
_In_ WDFCONTEXT Context)
{
	SPM_EvtRequestCompletionRoutine(Request, Target, CompletionParams, Context);
	return;
}

/*
* 완료 루틴
*/
VOID
SPM_EvtRequestCompletionRoutine(
_In_ WDFREQUEST Request,
_In_ WDFIOTARGET Target,
PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
_In_ WDFCONTEXT Context)
{
	NTSTATUS ntStatus;
	PDEVICE_EXTENSION devContext;
	WDFUSBPIPE	Pipe;
	UNREFERENCED_PARAMETER(Target);

	// WDF 오브젝트의 컨텍스트 얻음
	devContext = (PDEVICE_EXTENSION)Context;

#ifdef BULK
	if (devContext->is_read)
		Pipe = devContext->BulkINPipe;
	else
		Pipe = devContext->BulkOUTPipe;
#endif
#ifdef INTERRUPT
	if (devContext->is_read)
		Pipe = devContext->InterruptINPipe;
	else
		Pipe = devContext->InterruptOUTPipe;
#endif

	ntStatus = CompletionParams->IoStatus.Status;
	if (!NT_SUCCESS(ntStatus)){
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Request completed with failure: %X\n", ntStatus);
#endif
		IoFreeMdl(devContext->mdl);
		// If this was IOCTL-originated write we must unlock and free
		// our transfer MDL.
		if (devContext->is_ioctl && !devContext->is_read) {
			MmUnlockPages(devContext->transfer_mdl);
			IoFreeMdl(devContext->transfer_mdl);
		}
		WdfRequestComplete(Request, ntStatus);
		return;
	}

	// Get our URB buffer
	PURB urb = WdfMemoryGetBuffer(devContext->urb_mem, NULL);

	// Lets see how much has been transfered and update our counters accordingly
	ULONG bytes_transfered = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
	// We expect writes to transfer entire packet

	devContext->num_xfer += bytes_transfered;
	devContext->length -= bytes_transfered;

	// Is there anything left to transfer? Now, by the protocol we should
	// successfuly complete partial reads, instead of waiting on full set
	// of requested bytes being accumulated in the read buffer.
	if ((0 == devContext->length) || devContext->is_read) {
		ntStatus = STATUS_SUCCESS;

		// This was the last transfer
		if (devContext->is_ioctl && !devContext->is_read) {
			// For IOCTL-originated writes we have to return transfer size through
			// the IOCTL's output buffer.
			ULONG* ret_transfer = (ULONG*)(OutAddress(Request, NULL));
			ASSERT(NULL != ret_transfer);
			if (NULL != ret_transfer)
				*ret_transfer = devContext->num_xfer;
			WdfRequestSetInformation(Request, sizeof(ULONG));

			// We also must unlock / free transfer MDL
			MmUnlockPages(devContext->transfer_mdl);
			IoFreeMdl(devContext->transfer_mdl);
		}
		else {
			// For other requests we report transfer size through the request I/O
			// completion status.
			WdfRequestSetInformation(Request, devContext->num_xfer);
		}
		IoFreeMdl(devContext->mdl);
		WdfRequestComplete(Request, ntStatus);
		return;
	}

	// There are something left for the transfer. Prepare for it.
	// Required to free any mapping made on the partial MDL and
	// reset internal MDL state.
	MmPrepareMdlForReuse(devContext->mdl);

	// Update our virtual address
	devContext->virtual_address = (char*)(devContext->virtual_address) + bytes_transfered;

	// Calculate size of this transfer
	ULONG stage_len =
		(devContext->length > devContext->BulkReadPipe_MaximumPacketSize) ? devContext->BulkReadPipe_MaximumPacketSize :
		devContext->length;

	IoBuildPartialMdl(devContext->transfer_mdl,
		devContext->mdl,
		devContext->virtual_address,
		stage_len);

	// Reinitialize the urb and context
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = stage_len;
	devContext->transfer_size = stage_len;

	// Format the request to send a URB to a USB pipe.
	ntStatus = WdfUsbTargetPipeFormatRequestForUrb(Pipe,
		Request,
		devContext->urb_mem,
		NULL);

	if (!NT_SUCCESS(ntStatus)) {
		if (devContext->is_ioctl && !devContext->is_read) {
			MmUnlockPages(devContext->transfer_mdl);
			IoFreeMdl(devContext->transfer_mdl);
		}
		IoFreeMdl(devContext->mdl);
		WdfRequestComplete(Request, ntStatus);
		return;
	}

	// Reset the completion routine
	WdfRequestSetCompletionRoutine(Request,
		CommonReadWriteCompletionEntry,
		devContext);

	// Send the request asynchronously.
	if (!WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(Pipe), WDF_NO_SEND_OPTIONS)) {
		if (devContext->is_ioctl && !devContext->is_read) {
			MmUnlockPages(devContext->transfer_mdl);
			IoFreeMdl(devContext->transfer_mdl);
		}
		ntStatus = WdfRequestGetStatus(Request);
		IoFreeMdl(devContext->mdl);
		WdfRequestComplete(Request, ntStatus);
	}

	return;
}









VOID
SPM_EvtIoInternalDeviceControl(
_In_ WDFQUEUE Queue,
_In_ WDFREQUEST Request,
_In_ size_t OutputBufferLength,
_In_ size_t InputBufferLength,
_In_ ULONG IoControlCode)
{
	WDFDEVICE			Device;
	PDEVICE_EXTENSION	devContext;
	NTSTATUS			ntStatus = STATUS_SUCCESS;
	PIRP Irp;
	PIO_STACK_LOCATION Stack;
	PDEVICE_OBJECT DeviceObject;

	//	PCONNECT_DATA		connectData;
	//	PMOUSE_ATTRIBUTES mouse_attributes;
	//	PUCHAR readReport;
	//	PVOID systemBuffer;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	Device = WdfIoQueueGetDevice(Queue);
	DeviceObject = WdfDeviceWdmGetDeviceObject(Device);
	devContext = GetDeviceContext(Device);

	Irp = WdfRequestWdmGetIrp(Request);
	Stack = IoGetCurrentIrpStackLocation(Irp);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - %s, Queue:0x%p, Request:0x%p\n", DbgHidInternalIoctlString(IoControlCode), Queue, Request);
#endif
	switch (IoControlCode) {
		//case IOCTL_INTERNAL_MOUSE_CONNECT:
		//	// Only allow one connection.
		//	if (devContext->UpperConnectData.ClassService != NULL) {
		//		ntStatus = STATUS_SHARING_VIOLATION;
		//		break;
		//	}



		//	ntStatus = WdfRequestRetrieveInputBuffer(Request, sizeof(CONNECT_DATA), &connectData, &length);

		//	if (!NT_SUCCESS(ntStatus)){
		//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestRetrieveInputBuffer failed %x\n", ntStatus);
		//		break;
		//	}


		//	/* mouclass.sys 에서 주소 얻음 */
		//	devContext->UpperConnectData = *connectData;



		//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//	//PIRP NewIrp;
		//	//ULONG IoControlCode;
		//	//PDEVICE_OBJECT TopOfStack = NULL;
		//	//PVOID InputBuffer;
		//	//ULONG InputBufferLength;
		//	//KEVENT Event;
		//	//IO_STATUS_BLOCK IoStatus;

		//	///* IOCTL_INTERNAL_I8042_HOOK_MOUSE 호출 하라고 mouclass.sys에 알림 */
		//	//IoControlCode = IOCTL_INTERNAL_MOUSE_ENABLE;
		//	//TopOfStack = IoGetAttachedDeviceReference(DeviceObject);

		//	//
		//	//KeInitializeEvent(&Event, NotificationEvent, FALSE);

		//	//NewIrp = IoBuildDeviceIoControlRequest(
		//	//	IoControlCode,
		//	//	TopOfStack,
		//	//	NULL,
		//	//	0,
		//	//	NULL,
		//	//	0,
		//	//	TRUE,
		//	//	&Event,
		//	//	&IoStatus);

		//	//if (!NewIrp) {
		//	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - IoBuildDeviceIoControlRequest() failed\n");
		//	//}

		//	//ntStatus = IoCallDriver(TopOfStack, NewIrp);

		//	//if (!NT_SUCCESS(ntStatus)) {
		//	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - IoCallDriver() failed with status 0x%08lx\n", ntStatus);
		//	//}

		//	devContext->AttributesInformation.MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
		//	devContext->AttributesInformation.NumberOfButtons = 5;

		//	/* sample rate not used for usb */
		//	devContext->AttributesInformation.SampleRate = 0;
		//	devContext->AttributesInformation.InputDataQueueLength = 2;

		//	///* IOCTL_MOUSE_QUERY_ATTRIBUTES 호출  */
		//	//IoControlCode = IOCTL_MOUSE_QUERY_ATTRIBUTES;
		//	//TopOfStack = IoGetAttachedDeviceReference(DeviceObject);

		//	//InputBuffer = &devContext->AttributesInformation;
		//	//InputBufferLength = sizeof(MOUSE_ATTRIBUTES);

		//	//KeInitializeEvent(&Event, NotificationEvent, FALSE);

		//	//NewIrp = IoBuildDeviceIoControlRequest(
		//	//	IoControlCode,
		//	//	TopOfStack,
		//	//	InputBuffer,
		//	//	InputBufferLength,
		//	//	NULL,
		//	//	0,
		//	//	TRUE,
		//	//	&Event,
		//	//	&IoStatus);

		//	//if (!NewIrp) {
		//	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - IoBuildDeviceIoControlRequest() failed\n");
		//	//}

		//	//ntStatus = IoCallDriver(TopOfStack, NewIrp);

		//	//if (!NT_SUCCESS(ntStatus)) {
		//	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - IoCallDriver() failed with status 0x%08lx\n", ntStatus);
		//	//}

		//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//	ntStatus = STATUS_SUCCESS;

		//	//connectData->ClassDeviceObject = WdfDeviceWdmGetDeviceObject(Device);
		//	//connectData->ClassService = MouFilter_ServiceCallback;

		//	break;
		//case IOCTL_INTERNAL_MOUSE_DISCONNECT:
		//	//
		//	// Clear the connection parameters in the device extension.
		//	//
		//	// devExt->UpperConnectData.ClassDeviceObject = NULL;
		//	// devExt->UpperConnectData.ClassService = NULL;

		//	ntStatus = STATUS_SUCCESS;
		//	break;
		//case IOCTL_MOUSE_QUERY_ATTRIBUTES:	// Mouse 정보 물어보는 IOCTL

		//	ntStatus = WdfRequestRetrieveInputBuffer(Request, sizeof(MOUSE_ATTRIBUTES), &mouse_attributes, &length);
		//	if (!NT_SUCCESS(ntStatus)){
		//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestRetrieveInputBuffer failed %x\n", ntStatus);
		//		break;
		//	}


		//	if (length >= sizeof(MOUSE_ATTRIBUTES)) {
		//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");

		//		/* get output buffer */
		//		mouse_attributes = Irp->AssociatedIrp.SystemBuffer;

		//		mouse_attributes->InputDataQueueLength = devContext->AttributesInformation.InputDataQueueLength;
		//		mouse_attributes->MouseIdentifier = devContext->AttributesInformation.MouseIdentifier;
		//		mouse_attributes->NumberOfButtons = devContext->AttributesInformation.NumberOfButtons;
		//		mouse_attributes->SampleRate = devContext->AttributesInformation.SampleRate;

		//		//*(PMOUSE_ATTRIBUTES)Irp->AssociatedIrp.SystemBuffer = devContext->AttributesInformation;
		//		//Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);

		//		ntStatus = STATUS_SUCCESS;
		//	}
		//	else {
		//		ntStatus = STATUS_BUFFER_TOO_SMALL;
		//	}
		//	break;;
		//case IOCTL_INTERNAL_MOUSE_ENABLE:
		//	ntStatus = STATUS_PENDING;
		//	break;
		//case IOCTL_INTERNAL_MOUSE_DISABLE:
		//	ntStatus = STATUS_PENDING;
		//	break;



	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		//
		// Retrieves the device's HID descriptor.
		//
		ntStatus = SPM_GetHidDescriptor(Device, Request);
		break;

	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		//
		//Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
		//
		ntStatus = SPM_GetDeviceAttributes(Request);
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		//
		//Obtains the report descriptor for the HID device.
		//
		ntStatus = SPM_GetReportDescriptor(Device, Request);
		break;

	case IOCTL_HID_READ_REPORT:
		//
		// Returns a report from the device into a class driver-supplied buffer.
		// For now queue the request to the manual queue. The request will
		// be retrived and completd when continuous reader reads new data
		// from the device.
		//

		ntStatus = WdfRequestForwardToIoQueue(Request, devContext->InterruptMsgQueue);

		if (!NT_SUCCESS(ntStatus)){
#ifndef DBG
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestForwardToIoQueue failed with ntStatus: 0x%x\n", ntStatus);
#endif

			WdfRequestComplete(Request, ntStatus);
		}

		return;

		//
		// This feature is only supported on WinXp and later. Compiling in W2K 
		// build environment will fail without this conditional preprocessor statement.
		//
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		//
		// Hidclass sends this IOCTL for devices that have opted-in for Selective
		// Suspend feature. This feature is enabled by adding a registry value
		// "SelectiveSuspendEnabled" = 1 in the hardware key through inf file 
		// (see hidusbfx2.inf). Since hidclass is the power policy owner for 
		// this stack, it controls when to send idle notification and when to 
		// cancel it. This IOCTL is passed to USB stack. USB stack pends it. 
		// USB stack completes the request when it determines that the device is
		// idle. Hidclass's idle notification callback get called that requests a 
		// wait-wake Irp and subsequently powers down the device. 
		// The device is powered-up either when a handle is opened for the PDOs 
		// exposed by hidclass, or when usb stack completes wait
		// wake request. In the first case, hidclass cancels the notification 
		// request (pended with usb stack), cancels wait-wake Irp and powers up
		// the device. In the second case, an external wake event triggers completion
		// of wait-wake irp and powering up of device.
		//
		//ntStatus = HidFx2SendIdleNotification(Request);
		//
		//if (!NT_SUCCESS(ntStatus)) {
		//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SendIdleNotification failed with ntStatus: 0x%x\n", ntStatus);
		//
		//	WdfRequestComplete(Request, ntStatus);
		//}
		//
		return;

	case IOCTL_HID_SET_FEATURE:
		//
		// This sends a HID class feature report to a top-level collection of
		// a HID class device.
		//
		//ntStatus = HidFx2SetFeature(Request);
		//WdfRequestComplete(Request, ntStatus);
		return;

	case IOCTL_HID_GET_FEATURE:
		//
		// Get a HID class feature report from a top-level collection of
		// a HID class device.
		//
		//ntStatus = HidFx2GetFeature(Request, &bytesReturned);
		//WdfRequestCompleteWithInformation(Request, ntStatus, bytesReturned);
		return;

	case IOCTL_HID_WRITE_REPORT:
		//
		//Transmits a class driver-supplied report to the device.
		//
		break;
	case IOCTL_HID_GET_STRING:
		//
		// Requests that the HID minidriver retrieve a human-readable string
		// for either the manufacturer ID, the product ID, or the serial number
		// from the string descriptor of the device. The minidriver must send
		// a Get String Descriptor request to the device, in order to retrieve
		// the string descriptor, then it must extract the string at the
		// appropriate index from the string descriptor and return it in the
		// output buffer indicated by the IRP. Before sending the Get String
		// Descriptor request, the minidriver must retrieve the appropriate
		// index for the manufacturer ID, the product ID or the serial number
		// from the device extension of a top level collection associated with
		// the device.
		//
		break;
	case IOCTL_HID_ACTIVATE_DEVICE:
		//
		// Makes the device ready for I/O operations.
		//
		break;
	case IOCTL_HID_DEACTIVATE_DEVICE:
		//
		// Causes the device to cease operations and terminate all outstanding
		// I/O requests.
		//
		break;

	default:
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - IOCTL_Default CALL\n");
#endif
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	WdfRequestComplete(Request, ntStatus);

	return;
}


NTSTATUS
SPM_GetHidDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request)
{
	NTSTATUS            ntStatus = STATUS_SUCCESS;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetHidDescriptor Entry\n");
#endif
	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	ntStatus = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestRetrieveOutputMemory failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = G_DefaultHidDescriptor.bLength;

	if (bytesToCopy == 0) {
		ntStatus = STATUS_INVALID_DEVICE_STATE;
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - G_DefaultHidDescriptor is zero, 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	ntStatus = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PVOID)&G_DefaultHidDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfMemoryCopyFromBuffer failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetHidDescriptor Exit = 0x%x\n", ntStatus);
#endif
	return ntStatus;
}

NTSTATUS
SPM_GetDeviceAttributes(
IN WDFREQUEST Request)
{
	NTSTATUS                 ntStatus = STATUS_SUCCESS;
	PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;
	PUSB_DEVICE_DESCRIPTOR   usbDeviceDescriptor = NULL;
	PDEVICE_EXTENSION        devContext = NULL;
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetDeviceAttributes Entry\n");
#endif	
	devContext = GetDeviceContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request)));

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	ntStatus = WdfRequestRetrieveOutputBuffer(Request,
		sizeof (HID_DEVICE_ATTRIBUTES),
		&deviceAttributes,
		NULL);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestRetrieveOutputBuffer failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	//
	// Retrieve USB device descriptor saved in device context
	//
	usbDeviceDescriptor = WdfMemoryGetBuffer(devContext->UsbDeviceDescriptor, NULL);

	deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = usbDeviceDescriptor->idVendor;
	deviceAttributes->ProductID = usbDeviceDescriptor->idProduct;;
	deviceAttributes->VersionNumber = usbDeviceDescriptor->bcdDevice;

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetDeviceAttributes Exit = 0x%x\n", ntStatus);
#endif
	return ntStatus;
}


NTSTATUS
SPM_GetReportDescriptor(
IN WDFDEVICE Device,
IN WDFREQUEST Request)
{
	NTSTATUS            ntStatus = STATUS_SUCCESS;
	ULONG_PTR           bytesToCopy;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetReportDescriptor Entry\n");
#endif
	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	ntStatus = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfRequestRetrieveOutputMemory failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	//
	// Use hardcoded Report descriptor
	//
	bytesToCopy = G_DefaultHidDescriptor.DescriptorList[0].wReportLength;

	if (bytesToCopy == 0) {
		ntStatus = STATUS_INVALID_DEVICE_STATE;
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - G_DefaultHidDescriptor's reportLenght is zero, 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	ntStatus = WdfMemoryCopyFromBuffer(memory,
		0,
		(PVOID)G_DefaultReportDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfMemoryCopyFromBuffer failed 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SPM_GetReportDescriptor Exit = 0x%x\n", ntStatus);
#endif
	return ntStatus;
}










//
//NTSTATUS
//HidFx2SetFeature(
//IN WDFREQUEST Request
//)
///*++
//
//Routine Description
//
//This routine sets the state of the Feature: in this
//case Segment Display on the USB FX2 board.
//
//Arguments:
//
//Request - Wdf Request
//
//Return Value:
//
//NT ntStatus value
//
//--*/
//{
//	NTSTATUS                     ntStatus = STATUS_SUCCESS;
//	PHID_XFER_PACKET             transferPacket = NULL;
//	WDF_REQUEST_PARAMETERS       params;
//	PHIDFX2_FEATURE_REPORT       featureReport = NULL;
//	WDFDEVICE                    device;
//	UCHAR                        featureUsage = 0;
//
//	PAGED_CODE();
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HidFx2SetFeature Enter\n");
//
//	device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));
//
//	WDF_REQUEST_PARAMETERS_INIT(&params);
//	WdfRequestGetParameters(Request, &params);
//
//	//
//	// IOCTL_HID_SET_FEATURE & IOCTL_HID_GET_FEATURE are not METHOD_NIEHTER
//	// IOCTLs. So you cannot retreive UserBuffer from the IRP using Wdf
//	// function. As a result we have to escape out to WDM to get the UserBuffer
//	// directly from the IRP. 
//	//
//	if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) {
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Userbuffer is small 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	//
//	// This is a kernel buffer so no need for try/except block when accesssing
//	// Irp->UserBuffer.
//	//
//	transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
//	if (transferPacket == NULL) {
//		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Irp->UserBuffer is NULL 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	if (transferPacket->reportBufferLen == 0){
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HID_XFER_PACKET->reportBufferLen is 0, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	if (transferPacket->reportBufferLen < sizeof(UCHAR)){
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HID_XFER_PACKET->reportBufferLen is too small, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	featureReport = (PHIDFX2_FEATURE_REPORT)transferPacket->reportBuffer;
//	featureUsage = featureReport->FeatureData;
//
//	//
//	// The feature reports map directly to the command
//	// data that is sent down to the device.
//	//
//	if (transferPacket->reportId == SEVEN_SEGMENT_REPORT_ID)
//	{
//		ntStatus = SendVendorCommand(
//			device,
//			HIDFX2_SET_7SEGMENT_DISPLAY,
//			&featureUsage
//			);
//	}
//	else if (transferPacket->reportId == BARGRAPH_REPORT_ID)
//	{
//		ntStatus = SendVendorCommand(
//			device,
//			HIDFX2_SET_BARGRAPH_DISPLAY,
//			&featureUsage
//			);
//	}
//	else
//	{
//		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Incorrect report ID, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HidFx2SetFeature Exit\n");
//	return ntStatus;
//}
//
//NTSTATUS
//HidFx2GetFeature(
//IN WDFREQUEST Request,
//OUT PULONG BytesReturned
//)
///*++
//
//Routine Description
//
//This routine gets the state of the Feature: in this
//case Segment Display or bargraph display on the USB FX2 board.
//
//Arguments:
//
//Request - Wdf Request
//
//BytesReturned - Size of buffer returned for the request
//
//Return Value:
//
//NT ntStatus value
//
//--*/
//{
//	NTSTATUS                     ntStatus = STATUS_SUCCESS;
//	PHID_XFER_PACKET             transferPacket = NULL;
//	WDF_REQUEST_PARAMETERS       params;
//	PHIDFX2_FEATURE_REPORT       featureReport = NULL;
//	WDFDEVICE                    device;
//
//	PAGED_CODE();
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HidFx2GetFeature Enter\n");
//
//	*BytesReturned = 0;
//
//	device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));
//
//	WDF_REQUEST_PARAMETERS_INIT(&params);
//	WdfRequestGetParameters(Request, &params);
//
//	//
//	// IOCTL_HID_SET_FEATURE & IOCTL_HID_GET_FEATURE are not METHOD_NIEHTER
//	// IOCTLs. So you cannot retreive UserBuffer from the IRP using Wdf
//	// function. As a result we have to escape out to WDM to get the UserBuffer
//	// directly from the IRP. 
//	//
//	if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET)) {
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Userbuffer is small 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	//
//	// This is a kernel buffer so no need for try/except block when accesssing
//	// Irp->UserBuffer.
//	//
//	transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;
//	if (transferPacket == NULL) {
//		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Irp->UserBuffer is NULL 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	if (transferPacket->reportBufferLen == 0){
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HID_XFER_PACKET->reportBufferLen is 0, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	if (transferPacket->reportBufferLen < sizeof(UCHAR)){
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HID_XFER_PACKET->reportBufferLen is too small, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	featureReport = (PHIDFX2_FEATURE_REPORT)transferPacket->reportBuffer;
//
//	if (transferPacket->reportId == SEVEN_SEGMENT_REPORT_ID)
//	{
//		ntStatus = GetVendorData(
//			device,
//			HIDFX2_READ_7SEGMENT_DISPLAY,
//			&featureReport->FeatureData
//			);
//	}
//	else if (transferPacket->reportId == BARGRAPH_REPORT_ID)
//	{
//		ntStatus = GetVendorData(
//			device,
//			HIDFX2_READ_BARGRAPH_DISPLAY,
//			&featureReport->FeatureData
//			);
//	}
//	else
//	{
//		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Incorrect report ID, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	*BytesReturned = sizeof (HIDFX2_FEATURE_REPORT);
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - HidFx2GetFeature Exit\n");
//	return ntStatus;
//}
//
//NTSTATUS
//SendVendorCommand(
//IN WDFDEVICE Device,
//IN UCHAR VendorCommand,
//IN PUCHAR CommandData
//)
///*++
//
//Routine Description
//
//This routine sets the state of the Feature: in this
//case Segment Display on the USB FX2 board.
//
//Arguments:
//
//Request - Wdf Request
//
//Return Value:
//
//NT ntStatus value
//
//--*/
//{
//	NTSTATUS                     ntStatus = STATUS_SUCCESS;
//	ULONG                        bytesTransferred = 0;
//	PDEVICE_EXTENSION            pDevContext = NULL;
//	WDF_MEMORY_DESCRIPTOR        memDesc;
//	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
//	WDF_REQUEST_SEND_OPTIONS     sendOptions;
//
//	PAGED_CODE();
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SendVendorCommand Enter\n");
//
//	pDevContext = GetDeviceContext(Device);
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SendVendorCommand: Command:0x%x, data: 0x%x\n",
//		VendorCommand, *CommandData);
//
//	//
//	// set the segment state on the USB device
//	//
//	// Send the I/O with a timeout. We do that because we send the
//	// I/O in the context of the user thread and if it gets stuck, it would
//	// prevent the user process from existing. 
//	//
//	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
//		WDF_REQUEST_SEND_OPTION_TIMEOUT);
//
//	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
//		WDF_REL_TIMEOUT_IN_SEC(5));
//
//	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
//		BmRequestHostToDevice,
//		BmRequestToDevice,
//		VendorCommand, // Request
//		0, // Value
//		0); // Index
//
//	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
//		CommandData,
//		sizeof(UCHAR));
//
//	ntStatus = WdfUsbTargetDeviceSendControlTransferSynchronously(
//		pDevContext->UsbDevice,
//		WDF_NO_HANDLE, // Optional WDFREQUEST
//		&sendOptions, // PWDF_REQUEST_SEND_OPTIONS
//		&controlSetupPacket,
//		&memDesc,
//		&bytesTransferred
//		);
//
//	if (!NT_SUCCESS(ntStatus)) {
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SendtVendorCommand: Failed to set Segment Display state - 0x%x \n",
//			ntStatus);
//	}
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - SendVendorCommand Exit\n");
//
//	return ntStatus;
//}
//
//NTSTATUS
//GetVendorData(
//IN WDFDEVICE Device,
//IN UCHAR VendorCommand,
//IN PUCHAR CommandData
//)
///*++
//
//Routine Description
//
//This routine sets the state of the Feature: in this
//case Segment Display on the USB FX2 board.
//
//Arguments:
//
//Request - Wdf Request
//
//Return Value:
//
//NT ntStatus value
//
//--*/
//{
//	NTSTATUS                     ntStatus = STATUS_SUCCESS;
//	ULONG                        bytesTransferred = 0;
//	PDEVICE_EXTENSION            pDevContext = NULL;
//	WDF_MEMORY_DESCRIPTOR        memDesc;
//	WDF_USB_CONTROL_SETUP_PACKET controlSetupPacket;
//	WDF_REQUEST_SEND_OPTIONS     sendOptions;
//
//	PAGED_CODE();
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - GetVendorData Enter\n");
//
//	pDevContext = GetDeviceContext(Device);
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - GetVendorData: Command:0x%x, data: 0x%x\n",
//		VendorCommand, *CommandData);
//
//	//
//	// Get the display state from the USB device
//	//
//	// Send the I/O with a timeout. We do that because we send the
//	// I/O in the context of the user thread and if it gets stuck, it would
//	// prevent the user process from existing. 
//	//
//	WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
//		WDF_REQUEST_SEND_OPTION_TIMEOUT);
//
//	WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
//		WDF_REL_TIMEOUT_IN_SEC(5));
//
//	WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
//		BmRequestDeviceToHost,
//		BmRequestToDevice,
//		VendorCommand, // Request
//		0, // Value
//		0); // Index
//
//	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
//		CommandData,
//		sizeof(UCHAR));
//
//	ntStatus = WdfUsbTargetDeviceSendControlTransferSynchronously(
//		pDevContext->UsbDevice,
//		WDF_NO_HANDLE, // Optional WDFREQUEST
//		&sendOptions, // PWDF_REQUEST_SEND_OPTIONS
//		&controlSetupPacket,
//		&memDesc,
//		&bytesTransferred
//		);
//
//	if (!NT_SUCCESS(ntStatus)) {
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - GetVendorData: Failed to get state - 0x%x \n",
//			ntStatus);
//	}
//	else
//	{
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - GetVendorData: Command:0x%x, data after command: 0x%x\n",
//			VendorCommand, *CommandData);
//	}
//
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - GetVendorData Exit\n");
//
//	return ntStatus;
//}
//




//
//
//NTSTATUS
//HidFx2SendIdleNotification(
//IN WDFREQUEST Request
//)
///*++
//
//Routine Description:
//
//Pass down Idle notification request to lower driver
//
//Arguments:
//
//Request - Pointer to Request object.
//
//Return Value:
//
//NT ntStatus code.
//
//--*/
//{
//	NTSTATUS                   ntStatus = STATUS_SUCCESS;
//	BOOLEAN                    sendntStatus = FALSE;
//	WDF_REQUEST_SEND_OPTIONS   options;
//	WDFIOTARGET                nextLowerDriver;
//	WDFDEVICE                  device;
//	PIO_STACK_LOCATION         currentIrpStack = NULL;
//	IO_STACK_LOCATION          nextIrpStack;
//
//	device = WdfIoQueueGetDevice(WdfRequestGetIoQueue(Request));
//	currentIrpStack = IoGetCurrentIrpStackLocation(WdfRequestWdmGetIrp(Request));
//
//	//
//	// Convert the request to corresponding USB Idle notification request
//	//
//	if (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
//		sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)) {
//
//		ntStatus = STATUS_BUFFER_TOO_SMALL;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - DeviceIoControl.InputBufferLength too small, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	ASSERT(sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO)
//		== sizeof(USB_IDLE_CALLBACK_INFO));
//
//#pragma warning(suppress :4127)  // conditional expression is constant warning
//	if (sizeof(HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) != sizeof(USB_IDLE_CALLBACK_INFO)) {
//
//		ntStatus = STATUS_INFO_LENGTH_MISMATCH;
//		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Incorrect DeviceIoControl.InputBufferLength, 0x%x\n", ntStatus);
//		return ntStatus;
//	}
//
//	//
//	// prepare next stack location
//	//
//	RtlZeroMemory(&nextIrpStack, sizeof(IO_STACK_LOCATION));
//
//	nextIrpStack.MajorFunction = currentIrpStack->MajorFunction;
//	nextIrpStack.Parameters.DeviceIoControl.InputBufferLength =
//		currentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
//	nextIrpStack.Parameters.DeviceIoControl.Type3InputBuffer =
//		currentIrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
//	nextIrpStack.Parameters.DeviceIoControl.IoControlCode =
//		IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
//	nextIrpStack.DeviceObject =
//		WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(device));
//
//	//
//	// Format the I/O request for the driver's local I/O target by using the
//	// contents of the specified WDM I/O stack location structure.
//	//
//	WdfRequestWdmFormatUsingStackLocation(
//		Request,
//		&nextIrpStack
//		);
//
//	//
//	// Send the request down using Fire and forget option.
//	//
//	WDF_REQUEST_SEND_OPTIONS_INIT(
//		&options,
//		WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET
//		);
//
//	nextLowerDriver = WdfDeviceGetIoTarget(device);
//
//	sendntStatus = WdfRequestSend(
//		Request,
//		nextLowerDriver,
//		&options
//		);
//
//	if (sendntStatus == FALSE) {
//		ntStatus = STATUS_UNSUCCESSFUL;
//	}
//
//	return ntStatus;
//}



PCHAR
DbgHidInternalIoctlString(
IN ULONG IoControlCode)
{
	switch (IoControlCode)
	{
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
	case IOCTL_HID_READ_REPORT:
		return "IOCTL_HID_READ_REPORT";
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
	case IOCTL_HID_WRITE_REPORT:
		return "IOCTL_HID_WRITE_REPORT";
	case IOCTL_HID_SET_FEATURE:
		return "IOCTL_HID_SET_FEATURE";
	case IOCTL_HID_GET_FEATURE:
		return "IOCTL_HID_GET_FEATURE";
	case IOCTL_HID_GET_STRING:
		return "IOCTL_HID_GET_STRING";
	case IOCTL_HID_ACTIVATE_DEVICE:
		return "IOCTL_HID_ACTIVATE_DEVICE";
	case IOCTL_HID_DEACTIVATE_DEVICE:
		return "IOCTL_HID_DEACTIVATE_DEVICE";
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
	default:
		return "Unknown IOCTL";
	}
}


