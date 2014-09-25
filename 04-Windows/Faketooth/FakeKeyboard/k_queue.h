
#define TEST_BOARD_TRANSFER_BUFFER_SIZE (64*1024)


VOID FakeKeyboard_EvtRequestCompletionRoutine(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context);
NTSTATUS CommonBulkReadWrite(WDFQUEUE Queue, WDFREQUEST Request, PMDL transfer_mdl, ULONG length, BOOLEAN is_read, ULONG time_out, BOOLEAN is_ioctl);
VOID CommonReadWriteCompletionEntry(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context);
VOID FakeKeyboard_DispatchPassThrough(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target);
PCHAR DbgHidInternalIoctlString(IN ULONG IoControlCode);

NTSTATUS FakeKeyboard_GetHidDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS FakeKeyboard_GetDeviceAttributes(IN WDFREQUEST Request);
NTSTATUS FakeKeyboard_GetReportDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request);



__forceinline void* GetAddress(WDFMEMORY wdf_mem) {
	ASSERT(NULL != wdf_mem);
	return (NULL != wdf_mem) ? WdfMemoryGetBuffer(wdf_mem, NULL) : NULL;
}

__forceinline void* OutAddress(WDFREQUEST request, NTSTATUS* status) {

	WDFMEMORY wdf_mem = NULL;
	NTSTATUS stat = WdfRequestRetrieveOutputMemory(request, &wdf_mem);

	if (NULL != status)
		*status = stat;
	return NT_SUCCESS(stat) ? GetAddress(wdf_mem) : NULL;
}



CONST  HID_REPORT_DESCRIPTOR           hid_descriptor_keyboard[] = {
	/****/ 0x05, 0x01,     /* USAGE_PAGE (Generic Desktop) */
	/****/ 0x09, 0x06,     /* USAGE (Keyboard) */
	/****/ 0xa1, 0x01,     /* COLLECTION (Application) */
	/******/ 0x85, REPORTID_KEYBOARD,   /*   REPORT_ID (REPORTID_KEYBOARD = 0x01) */
	/******/ 0x05, 0x07,   /*   USAGE_PAGE (Keyboard) */
	/* Ctrl, Shift and other modifier keys, 8 in total */
	/******/ 0x19, 0xe0,   /*   USAGE_MINIMUM (kbd LeftControl) */
	/******/ 0x29, 0xe7,   /*   USAGE_MAXIMUM (kbd Right GUI) */
	/******/ 0x15, 0x00,   /*   LOGICAL_MINIMUM (0) */
	/******/ 0x25, 0x01,   /*   LOGICAL_MAXIMUM (1) */
	/******/ 0x75, 0x01,   /*   REPORT_SIZE (1) */
	/******/ 0x95, 0x08,   /*   REPORT_COUNT (8) */
	/******/ 0x81, 0x02,   /*   INPUT (Data,Var,Abs) */
	/* Reserved byte */
	/******/ 0x95, 0x01,   /*   REPORT_COUNT (1) */
	/******/ 0x75, 0x08,   /*   REPORT_SIZE (8) */
	/******/ 0x81, 0x01,   /*   INPUT (Cnst,Ary,Abs) */
	/* LEDs for num lock etc */
	/******/ 0x95, 0x05,   /*   REPORT_COUNT (5) */
	/******/ 0x75, 0x01,   /*   REPORT_SIZE (1) */
	/******/ 0x05, 0x08,   /*   USAGE_PAGE (LEDs) */
	/******/ 0x85, 0x01,   /*   REPORT_ID (1) */
	/******/ 0x19, 0x01,   /*   USAGE_MINIMUM (Num Lock) */
	/******/ 0x29, 0x05,   /*   USAGE_MAXIMUM (Kana) */
	/******/ 0x91, 0x02,   /*   OUTPUT (Data,Var,Abs) */
	/* Reserved 3 bits */
	/******/ 0x95, 0x01,   /*   REPORT_COUNT (1) */
	/******/ 0x75, 0x03,   /*   REPORT_SIZE (3) */
	/******/ 0x91, 0x03,   /*   OUTPUT (Cnst,Var,Abs) */
	/* Slots for 6 keys that can be pressed down at the same time */
	/******/ 0x95, 0x06,   /*   REPORT_COUNT (6) */
	/******/ 0x75, 0x08,   /*   REPORT_SIZE (8) */
	/******/ 0x15, 0x00,   /*   LOGICAL_MINIMUM (0) */
	/******/ 0x25, 0x65,   /*   LOGICAL_MAXIMUM (101) */
	/******/ 0x05, 0x07,   /*   USAGE_PAGE (Keyboard) */
	/******/ 0x19, 0x00,   /*   USAGE_MINIMUM (Reserved (no event indicated)) */
	/******/ 0x29, 0x65,   /*   USAGE_MAXIMUM (Keyboard Application) */
	/******/ 0x81, 0x00,   /*   INPUT (Data,Ary,Abs) */
	/****/ 0xc0
};

//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of hid_descriptor_keyboard.
//
CONST HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0101, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(hid_descriptor_keyboard) }  // total length of report descriptor
};
