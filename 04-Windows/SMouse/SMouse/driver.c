#include "driver.h"




#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, SPM_DeviceAdd)
#pragma alloc_text( PAGE, SPM_EvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
_In_ PDRIVER_OBJECT  DriverObject,
_In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS               ntStatus = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n\n=== HID MiniDriver - Hello World %s %s\n", __DATE__, __TIME__);
#endif 

	// WDF_DRIVER_CONFIG 구조체 설정
	WDF_DRIVER_CONFIG_INIT(&config, SPM_DeviceAdd);

	// WDF_OBJECT_ATTRIBUTES 구조체 설정
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.EvtCleanupCallback = SPM_EvtDriverContextCleanup;

	// WDFDRIVER 오브젝트 생성
	ntStatus = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,      // Driver Attributes
		&config,          // Driver Config Info
		WDF_NO_HANDLE
		);

	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - WdfDriverCreate failed with ntStatus 0x%x\n", ntStatus);
#endif
	}

	return ntStatus;
}

VOID
SPM_EvtDriverContextCleanup(
IN WDFOBJECT Object)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Object);
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - " "Exit SPM_EvtDriverContextCleanup\n");
#endif
}

NTSTATUS
SPM_DeviceAdd(
IN WDFDRIVER       Driver,
IN PWDFDEVICE_INIT DeviceInit)
{
	NTSTATUS						ntStatus = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES			attributes;
	WDFDEVICE						hDevice;
	PDEVICE_EXTENSION				devContext = NULL;
	WDF_PNPPOWER_EVENT_CALLBACKS	pnpPowerCallbacks;


	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();
#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID MiniDriver - Device Add === \n");
#endif

	// 필터 드라이버 라고 정의
	WdfFdoInitSetFilter(DeviceInit);

	// 마우스 타입이라 정의
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_MOUSE);

	// I/O Type : buffered (default)
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);
	//	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
	//	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoNeither);

	// 초기화 처리, PnP,power 콜백함수 등록
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	pnpPowerCallbacks.EvtDevicePrepareHardware = SPM_EvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = SPM_EvtDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceSurpriseRemoval = SPM_EvtDeviceSurpriseRemoval;

	// Continuous Reader Entry & Exit 콜백함수 등록
	pnpPowerCallbacks.EvtDeviceD0Entry = SPM_EvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = SPM_EvtDeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);


	// WDF_OBJECT_ATTRIBUTES 구조체 설정
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);


	// WDFDEVICE 오브젝트 생성
	ntStatus = WdfDeviceCreate(&DeviceInit, &attributes, &hDevice);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - WdfDeviceCreate failed with ntStatus code 0x%x\n", ntStatus);
#endif
		return ntStatus;
	}

	devContext = GetDeviceContext(hDevice);

	// WDF QUEUE 설정
	ntStatus = SPM_QueueInitialize(hDevice);

	// GUID 등록
	ntStatus = WdfDeviceCreateDeviceInterface(hDevice, (LPGUID)&SPM_DRIVER, NULL);
	if (!NT_SUCCESS(ntStatus)) {
#ifndef DBG
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - Create Device Interface Fail === \n");
#endif
		return ntStatus;
	}

#ifndef DBG
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== HID minidriver - Device Add End === \n");
#endif

	return ntStatus;
}




