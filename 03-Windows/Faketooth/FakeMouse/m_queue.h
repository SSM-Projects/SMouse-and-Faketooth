
#define TEST_BOARD_TRANSFER_BUFFER_SIZE (64*1024)


VOID FakeMouse_EvtRequestCompletionRoutine(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context);
NTSTATUS CommonBulkReadWrite(WDFQUEUE Queue, WDFREQUEST Request, PMDL transfer_mdl, ULONG length, BOOLEAN is_read, ULONG time_out, BOOLEAN is_ioctl);
VOID CommonReadWriteCompletionEntry(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target, PWDF_REQUEST_COMPLETION_PARAMS CompletionParams, _In_ WDFCONTEXT Context);
VOID FakeMouse_DispatchPassThrough(_In_ WDFREQUEST Request, _In_ WDFIOTARGET Target);
PCHAR DbgHidInternalIoctlString(IN ULONG IoControlCode);

NTSTATUS FakeMouse_GetHidDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS FakeMouse_GetDeviceAttributes(IN WDFREQUEST Request);
NTSTATUS FakeMouse_GetReportDescriptor(IN WDFDEVICE Device, IN WDFREQUEST Request);




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



CONST  HID_REPORT_DESCRIPTOR           hid_descriptor_mouse[] = {
	0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
	0x09, 0x02,                         // USAGE (Mouse)
	0xa1, 0x01,                         // COLLECTION (Application)
	0x85, REPORTID_MOUSE,				//   REPORT_ID (Mouse) (REPORTID_MOUSE = 0x01)
	0x09, 0x01,                         //   USAGE (Pointer)
	0xa1, 0x00,                         //   COLLECTION (Physical)
	0x05, 0x09,                         //     USAGE_PAGE (Button)
	0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)
	0x29, 0x03,                         //     USAGE_MAXIMUM (Button 3)
	0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
	0x75, 0x01,                         //     REPORT_SIZE (1)
	0x95, 0x03,                         //     REPORT_COUNT (3)
	0x81, 0x02,                         //     INPUT (Data,Var,Abs)
	0x95, 0x05,                         //     REPORT_COUNT (5)
	0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
	0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
	0x09, 0x30,                         //     USAGE (X)
	0x09, 0x31,                         //     USAGE (Y)
	0x09, 0x38,							//	   USAGE (Wheel)
	0x75, 0x10,                         //     REPORT_SIZE (16)
	0x95, 0x03,                         //     REPORT_COUNT (3)
	0x16, 0x01, 0x80,					//     LOGICAL_MINIMUM (-32767)
	0x26, 0xff, 0x7f,					//     LOGICAL_MAXIMUM (32767)
	0x81, 0x06,                         //     INPUT (Data,Var,Rel)
	0xc0,                               //   END_COLLECTION
	0xc0,                               // END_COLLECTION
};

//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of hid_descriptor_mouse.
//
CONST HID_DESCRIPTOR G_DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0101, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(hid_descriptor_mouse) }  // total length of report descriptor
};
