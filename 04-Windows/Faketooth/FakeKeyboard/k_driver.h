

/**********************************************************************/
/***********************        HEADER        *************************/
/**********************************************************************/
#pragma warning(disable:4055) // type case from PVOID to PSERVICE_CALLBACK_ROUTINE
#pragma warning(disable:4152) // function/data pointer conversion in expression


#include <initguid.h>
#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <wdf.h>
#include <wdfusb.h>
#include <hidport.h>
#include <ntstrsafe.h>
#include <pshpack1.h>

#include <kbdmou.h>



/**********************************************************************/
/***********************        DEFINE        *************************/
/**********************************************************************/
#define INTERRUPT
#define REPORTID_KEYBOARD 0x01
// #define DBGs

// HID GUID : {745a17a0-74d3-11d0-b6fe-00a0c90f57da} 
// DEFINE_GUID(FakeKeyboard_DRIVER, 0x745a17a0, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda);

// {88BAE032-5A81-49f0-BC3D-A4FF138216D6}
DEFINE_GUID(FakeKeyboard_DRIVER, 0x88BAE032, 0x5A81, 0x49F0, 0xBC, 0x3D, 0xA4, 0xFF, 0x13, 0x82, 0x16, 0xD6);

/**********************************************************************/
/*******************        TYPEDEF STRUCT        *********************/
/**********************************************************************/

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

typedef struct _HID_KEYBOARD_DATA
{
	UCHAR ID;
	UCHAR modifier; // bit flags for shift, ctrl, alt, and other stuff
	UCHAR reserved;	//
	UCHAR key[6];	// HID Key codes 
}HID_KEYBOARD_DATA, *PHID_KEYBOARD_DATA;

typedef struct _DEVICE_EXTENSION{

	WDFUSBDEVICE					UsbDevice;	//WDF handles for USB Target 
	WDFMEMORY			            UsbDeviceDescriptor;
	WDFUSBINTERFACE					UsbInterface;
	WDFUSBPIPE						BulkINPipe;   // Pipe opened for the bulk IN endpoint.
	WDFUSBPIPE						BulkOUTPipe;  // Pipe opened for the bulk OUT endpoint.
	WDFUSBPIPE						InterruptINPipe;  // Pipe opened for the interrupt IN endpoint.
	WDFUSBPIPE						InterruptOUTPipe;  // Pipe opened for the interrupt OUT endpoint.
	WDFQUEUE						IoReadQueue;
	WDFQUEUE						IoWriteQueue;
	WDFQUEUE						IoDefaultQueue;
	WDFQUEUE						IoMouseQueue;
	WDFQUEUE						ReadMsgQueue;
	WDFQUEUE						WriteMsgQueue;
	PUSB_CONFIGURATION_DESCRIPTOR	ConfigurationDescriptor;
	ULONG							BulkReadPipe_MaximumPacketSize;
	ULONG							InterruptPipe_MaximumPacketSize;
	UCHAR							NumberConfiguredPipes;  // Number of pipes opened.

	/* For Read & Write */
	LARGE_INTEGER			        sent_at;				// (time of the first WdfRequestSend is called for it)
	WDFMEMORY						urb_mem;				// KMDF descriptor for the memory allocated for URB
	PMDL							transfer_mdl;			// MDL describing the transfer buffer
	PMDL							mdl;					// Private MDL that we build in order to perform the transfer
	void*							virtual_address;		// Virtual address for the current segment of transfer.
	ULONG							length;					// Number of bytes remaining to transfer
	ULONG							transfer_size;			// Number of bytes requested to transfer
	ULONG							num_xfer;				// Accummulated number of bytes transferred
	ULONG							initial_time_out;		// Initial timeout (in millisec) set for this request
	BOOLEAN							is_read;				// Read / Write selector
	BOOLEAN							is_ioctl;				// IOCTL selector

	/* Mouse Function */
	CONNECT_DATA					UpperConnectData;
	MOUSE_ATTRIBUTES				AttributesInformation;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceContext)


/**********************************************************************/
/***********************        FUNTION        ************************/
/**********************************************************************/

/* driver */
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD		FakeKeyboard_DeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP	FakeKeyboard_EvtDriverContextCleanup;

/* pnp */
EVT_WDF_DEVICE_PREPARE_HARDWARE FakeKeyboard_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE FakeKeyboard_EvtDeviceReleaseHardware;
NTSTATUS FakeKeyboard_EvtDeviceSurpriseRemoval(_In_ WDFDEVICE Device);

EVT_WDF_DEVICE_D0_ENTRY FakeKeyboard_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT	FakeKeyboard_EvtDeviceD0Exit;

NTSTATUS FakeKeyboard_InitializePipeInformation(_In_ WDFDEVICE Device);
NTSTATUS FakeKeyboard_GetConfigurationDescriptor(_In_ WDFDEVICE Device);
NTSTATUS FakeKeyboard_InitPowerManagement(_In_ WDFDEVICE  Device);
PCHAR DbgDevicePowerString(IN WDF_POWER_DEVICE_STATE Type);


/* queue */
NTSTATUS FakeKeyboard_QueueInitialize(_In_ WDFDEVICE Device);

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL	FakeKeyboard_EvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ					FakeKeyboard_EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE					FakeKeyboard_EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEFAULT					FakeKeyboard_EvtIoDefault;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL			FakeKeyboard_EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP					FakeKeyboard_EvtIoStop;


/* reader */
NTSTATUS FakeKeyboard_ConfigureContinuousReader(_In_ WDFDEVICE Device);
EVT_WDF_USB_READER_COMPLETION_ROUTINE FakeKeyboard_EvtReadComplete;
EVT_WDF_USB_READERS_FAILED FakeKeyboard_EvtReadFailed;
VOID FakeKeyboard_KeyboardReport(PDEVICE_EXTENSION devContext, PVOID requestBuffer);



/**********************************************************************/
/******************        GLOBAL CARIABLE        *********************/
/**********************************************************************/
