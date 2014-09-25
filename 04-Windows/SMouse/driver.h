

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
#define REPORTID_MOUSE 0x01

// HID GUID : {745a17a0-74d3-11d0-b6fe-00a0c90f57da} 
DEFINE_GUID(SPM_DRIVER, 0x745a17a0, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda);

// USB GUID : {36FC9E60-C465-11CF-8056-444553540000}
//DEFINE_GUID(SPM_DRIVER, 0x36FC9E60, 0xC465, 0x11CF, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);

// Mouse GUID : {4D36E96F-E325-11CE-BFC1-08002BE10318}
//DEFINE_GUID(SPM_DRIVER, 0x4D36E96F, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);



/**********************************************************************/
/*******************        TYPEDEF STRUCT        *********************/
/**********************************************************************/

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

typedef struct _HID_MOUSE_DATA
{
	UCHAR ID;
	UCHAR buttons;
	USHORT lastX;
	USHORT lastY;
	USHORT wheel;
	// 16 bit·Î 3°³ 
}HID_MOUSE_DATA, *PHID_MOUSE_DATA;

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
	WDFQUEUE						InterruptMsgQueue;
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

EVT_WDF_DRIVER_DEVICE_ADD		SPM_DeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP	SPM_EvtDriverContextCleanup;

/* pnp */
EVT_WDF_DEVICE_PREPARE_HARDWARE SPM_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE SPM_EvtDeviceReleaseHardware;
NTSTATUS SPM_EvtDeviceSurpriseRemoval(_In_ WDFDEVICE Device);

EVT_WDF_DEVICE_D0_ENTRY SPM_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT	SPM_EvtDeviceD0Exit;

NTSTATUS SPM_InitializePipeInformation(_In_ WDFDEVICE Device);
NTSTATUS SPM_GetConfigurationDescriptor(_In_ WDFDEVICE Device);
NTSTATUS SPM_InitPowerManagement(_In_ WDFDEVICE  Device);
PCHAR DbgDevicePowerString(IN WDF_POWER_DEVICE_STATE Type);


/* queue */
NTSTATUS SPM_QueueInitialize(_In_ WDFDEVICE Device);

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL	SPM_EvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_READ					SPM_EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE					SPM_EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEFAULT					SPM_EvtIoDefault;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL			SPM_EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP					SPM_EvtIoStop;


/* reader */
NTSTATUS SPM_ConfigureContinuousReader(_In_ WDFDEVICE Device);
EVT_WDF_USB_READER_COMPLETION_ROUTINE SPM_EvtReadComplete;
EVT_WDF_USB_READERS_FAILED SPM_EvtReadFailed;
VOID SPM_MouseReport(PDEVICE_EXTENSION devContext, PVOID requestBuffer);

////////////////////////////////////////////////////////////////////////















/**********************************************************************/
/******************        GLOBAL CARIABLE        *********************/
/**********************************************************************/
