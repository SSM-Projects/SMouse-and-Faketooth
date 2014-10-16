#include "m_driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FakeMouse_EvtDevicePrepareHardware)
#pragma alloc_text(PAGE, FakeMouse_EvtDeviceD0Exit)

#endif

NTSTATUS
FakeMouse_EvtDevicePrepareHardware(
IN WDFDEVICE    Device,
IN WDFCMRESLIST ResourceList,
IN WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devContext;
	//PUSB_DEVICE_DESCRIPTOR              usbDeviceDescriptor = NULL;

	UNREFERENCED_PARAMETER(ResourceList);
	UNREFERENCED_PARAMETER(ResourceListTranslated);
	PAGED_CODE();
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Evt Device Prepare Hardware === \n");
#endif

	// WDFDEVICE 오브젝트의 컨텍스트 데이터를 가져옴
	devContext = GetDeviceContext(Device);


	if (devContext->UsbDevice == NULL) {
		// WDFUSBDEVICE 오브젝트 생성.
		ntStatus = WdfUsbTargetDeviceCreate(Device, WDF_NO_OBJECT_ATTRIBUTES, &devContext->UsbDevice);
		if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfUsbTargetDeviceCreate Failed 0x%x === \n", ntStatus);
#endif
			return ntStatus;
		}
	}

	// WDF USB PIPE 오브젝트를 얻음
	FakeMouse_InitializePipeInformation(Device);

	FakeMouse_GetConfigurationDescriptor(Device);

	// power Management Init
	FakeMouse_InitPowerManagement(Device);

	// Continuous Reader
	FakeMouse_ConfigureContinuousReader(Device);

	return ntStatus;
}

NTSTATUS
FakeMouse_EvtDeviceReleaseHardware(
_In_ WDFDEVICE Device,
_In_ WDFCMRESLIST ResourceListTranslated)
{
	NTSTATUS ntStatus;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
	PDEVICE_EXTENSION devContext;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(ResourceListTranslated);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Evt Device Release Hardware === \n");
#endif

	// WDFDEVICE 오브젝트의 컨텍스트 데이터 얻음
	devContext = GetDeviceContext(Device);

	// Deconfig 종료 처리
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG(&configParams);

	ntStatus = WdfUsbTargetDeviceSelectConfig(devContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);

	return STATUS_SUCCESS;
}


NTSTATUS
FakeMouse_EvtDeviceSurpriseRemoval(
_In_ WDFDEVICE Device)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Device);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Evt Device Surprise Removal === \n");
#endif

	return STATUS_SUCCESS;
}



/*
* pipe 정보 얻음
*/
NTSTATUS
FakeMouse_InitializePipeInformation(
_In_ WDFDEVICE Device)
{
	NTSTATUS								ntStatus;
	PDEVICE_EXTENSION						devContext;
	UCHAR									now;
	WDFUSBPIPE								pipe;
	WDF_USB_PIPE_INFORMATION				pipeInformation;
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS		configParams;
	WDF_OBJECT_ATTRIBUTES					attributes;
	PUSB_DEVICE_DESCRIPTOR					usbDeviceDescriptor = NULL;

	UNREFERENCED_PARAMETER(Device);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Initialize Pipe Information === \n");
#endif
	devContext = GetDeviceContext(Device);

	//devContext->UsbInterface = WdfUsbTargetDeviceGetInterface(devContext->UsbDevice, 0);

	// 싱글 디바이스로 초기화
	WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

	ntStatus = WdfUsbTargetDeviceSelectConfig(devContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfUsbTargetDeviceSelectConfig Failed 0x%x === \n", ntStatus);
#endif
		return ntStatus;
	}

	//// 멀티 디바이스로 초기화
	//WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&configParams, 0, NULL);

	//ntStatus = WdfUsbTargetDeviceSelectConfig(devContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);
	//if (!NT_SUCCESS(ntStatus)) {
	//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfUsbTargetDeviceSelectConfig failed, ntStatus : 0x%x\n", ntStatus);
	//	return ntStatus;
	//}


	// 컨피규레이션 디스크립터 얻음
	devContext->UsbInterface = configParams.Types.SingleInterface.ConfiguredUsbInterface;


	// 디바이스 디스크립터 얻음
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = Device;
	ntStatus = WdfMemoryCreate(
		&attributes,
		NonPagedPool,
		0,
		sizeof(USB_DEVICE_DESCRIPTOR),
		&devContext->UsbDeviceDescriptor,
		&usbDeviceDescriptor
		);

	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - WdfMemoryCreate for Device Descriptor failed %!STATUS!\n", ntStatus);
#endif
		return ntStatus;
	}

	WdfUsbTargetDeviceGetDeviceDescriptor(
		devContext->UsbDevice,
		usbDeviceDescriptor
		);


	// Get the number of pipes in the current altenrate setting.
	devContext->NumberConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(devContext->UsbInterface);


	if (devContext->NumberConfiguredPipes == 0) {
		ntStatus = USBD_STATUS_BAD_NUMBER_OF_ENDPOINTS;
		goto Exit;
	}
	else {
		ntStatus = STATUS_SUCCESS;
	}

	devContext->BulkINPipe = NULL;
	devContext->BulkOUTPipe = NULL;
	devContext->InterruptINPipe = NULL;
	devContext->InterruptOUTPipe = NULL;

	// 모든 파이프 검색
	// 벌크 IN, OUT의 WDFUSBPIPE 오브젝트를 WDFDEVICE 오브젝트의 컨텍스트 데이터에 설정
	for (now = 0; now < devContext->NumberConfiguredPipes; now++) {
		WDF_USB_PIPE_INFORMATION_INIT(&pipeInformation);

		// WDF USB INTERFACE로 부터 WDF USB PIPE를 얻음
		pipe = WdfUsbInterfaceGetConfiguredPipe(devContext->UsbInterface, now, &pipeInformation);
		if (pipe == NULL) {
			continue;
		}

		// maximum packet Size보다 작게 읽어도 괜찮음을 프레임워크에 알림
		WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pipe);

		if (pipeInformation.PipeType == WdfUsbPipeTypeBulk) {
			if (WDF_USB_PIPE_DIRECTION_IN(pipeInformation.EndpointAddress)) {
				devContext->BulkINPipe = pipe;
				devContext->BulkReadPipe_MaximumPacketSize = pipeInformation.MaximumPacketSize;
#ifdef DBGs
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"=== Mouse Driver - Bulk In, type:%d, EP:0x%x, Pipe:%p, MaximumPacketSize : %d === \n", pipeInformation.PipeType, pipeInformation.EndpointAddress, pipe, pipeInformation.MaximumPacketSize);
#endif
			}
			else {
				devContext->BulkOUTPipe = pipe;
#ifdef DBGs
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"=== Mouse Driver - Bulk Out, type:%d, EP:0x%x, Pipe:%p === \n", pipeInformation.PipeType, pipeInformation.EndpointAddress, pipe);
#endif
			}
			continue;
		}

		if (pipeInformation.PipeType == WdfUsbPipeTypeInterrupt) {
			if (WDF_USB_PIPE_DIRECTION_IN(pipeInformation.EndpointAddress)) {
				devContext->InterruptINPipe = pipe;
				devContext->InterruptPipe_MaximumPacketSize = pipeInformation.MaximumPacketSize;
#ifdef DBGs
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"=== Mouse Driver - Interrupt In, type:%d, EP:0x%x, Pipe:%p, MaximumPacketSize : %d === \n", pipeInformation.PipeType, pipeInformation.EndpointAddress, pipe, pipeInformation.MaximumPacketSize);
#endif
			}
			else {
				devContext->InterruptOUTPipe = pipe;
#ifdef DBGs
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
					"=== Mouse Driver - Interrupt Out, type:%d, EP:0x%x, Pipe:%p === \n", pipeInformation.PipeType, pipeInformation.EndpointAddress, pipe);
#endif
			}

			continue;
		}
	}

Exit:
	return ntStatus;
}


/*
* ConfigurationDescriptor 얻음
*/
NTSTATUS
FakeMouse_GetConfigurationDescriptor(
_In_ WDFDEVICE Device)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//	WDF_OBJECT_ATTRIBUTES attributes;
	PDEVICE_EXTENSION devContext;
	//	WDFMEMORY memory;

	UNREFERENCED_PARAMETER(Device);

	devContext = GetDeviceContext(Device);

	devContext->ConfigurationDescriptor = NULL;

	//// wTotalLength를 얻음
	//ntStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(devContext->UsbDevice, NULL, &devContext->wTotalLength);
	//// WdfUsbTargetDeviceRetrieveConfigDescriptor 함수의 ConfigDescriptor 인수에 NULL을 설정하면 STATUS_BUFFER_TOO_SMALL 반환됨.
	//if (ntStatus != STATUS_BUFFER_TOO_SMALL)
	//	return ntStatus;

	//// 부모 object로 WDFUSBDEVICE object 지정
	//WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	//attributes.ParentObject = devContext->UsbDevice;
	//


	//// wTotalLength만큼 메모리 할당
	//ntStatus = WdfMemoryCreate(&attributes, NonPagedPool, POOL_TAGGING, devContext->wTotalLength, &memory, &devContext->ConfigurationDescriptor);
	//if (!NT_SUCCESS(ntStatus))
	//	return ntStatus;

	//// 전체 컨피규레이션 디스크립터 얻음
	//ntStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(devContext->UsbDevice, devContext->ConfigurationDescriptor, &devContext->wTotalLength);
	//	
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Get Configuration Descriptor st=%x, bConfiguratopnValue=%d === \n",ntStatus,devContext->ConfigurationDescriptor->bConfigurationValue);

	//USB_STRING_DESCRIPTOR *pFullUSD;
	//URB urb;

	//
	//pFullUSD = ExAllocatePool(NonPagedPool, 0x40);
	//UsbBuildGetDescriptorRequest(
	//	&urb, // points to the URB to be filled in
	//	sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
	//	USB_STRING_DESCRIPTOR_TYPE,
	//	0x0003, // index of string descriptor
	//	0x0409, // language ID of string
	//	pFullUSD,
	//	NULL,
	//	0x40,
	//	NULL
	//	);

	WDFMEMORY  memoryHandle;
	USHORT  numCharacters;

	ntStatus = WdfUsbTargetDeviceAllocAndQueryString(
		devContext->UsbDevice,
		WDF_NO_OBJECT_ATTRIBUTES,
		&memoryHandle,
		&numCharacters,
		0x0003,
		0x0409
		);

	return ntStatus;
}



NTSTATUS
FakeMouse_InitPowerManagement(
_In_ WDFDEVICE  Device)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_EXTENSION devContext;
	WDF_USB_DEVICE_INFORMATION usbInfo;

#ifdef DBGs	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Device init power management\n");
#endif

	devContext = GetDeviceContext(Device);

	WDF_USB_DEVICE_INFORMATION_INIT(&usbInfo);
	ntStatus = WdfUsbTargetDeviceRetrieveInformation(devContext->UsbDevice, &usbInfo);
	if (!NT_SUCCESS(ntStatus))
	{
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
			"=== Mouse Driver - WdfUsbTargetDeviceRetrieveInformation failed with ntStatus 0x%08x\n", ntStatus);
#endif
		return ntStatus;
	}

#ifdef DBGs		
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Device self powered: %d\n",
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_SELF_POWERED ? 1 : 0);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Device remote wake capable: %d\n",
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE ? 1 : 0);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Device high speed: %d\n",
		usbInfo.Traits & WDF_USB_DEVICE_TRAIT_AT_HIGH_SPEED ? 1 : 0);
#endif

	if (usbInfo.Traits & WDF_USB_DEVICE_TRAIT_REMOTE_WAKE_CAPABLE)
	{
		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
		WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;

		WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings,
			IdleUsbSelectiveSuspend);
		idleSettings.IdleTimeout = 10000;
		ntStatus = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
		if (!NT_SUCCESS(ntStatus))
		{
#ifdef DBGs
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
				"=== Mouse Driver - WdfDeviceAssignS0IdleSettings failed with ntStatus 0x%08x\n", ntStatus);
#endif
			return ntStatus;
		}

		WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
		wakeSettings.DxState = PowerDeviceD2;
		ntStatus = WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);
		if (!NT_SUCCESS(ntStatus))
		{
#ifdef DBGs
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
				"=== Mouse Driver - WdfDeviceAssignSxWakeSettings failed with ntStatus 0x%08x\n", ntStatus);
#endif
			return ntStatus;
		}
	}

	return ntStatus;
}







NTSTATUS
FakeMouse_EvtDeviceD0Entry(
IN  WDFDEVICE Device,
IN  WDF_POWER_DEVICE_STATE PreviousState)
{
	PDEVICE_EXTENSION	devContext;
	NTSTATUS			ntStatus = STATUS_SUCCESS;

	PAGED_CODE();

	UNREFERENCED_PARAMETER(PreviousState);
#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - FakeMouse_EvtDeviceD0Entry Enter - coming from %s\n", DbgDevicePowerString(PreviousState));
#endif

	devContext = GetDeviceContext(Device);

#ifdef BULK
	ntStatus = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(devContext->BulkINPipe));
#endif
#ifdef INTERRUPT
	ntStatus = WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(devContext->InterruptINPipe));
#endif
	if (!NT_SUCCESS(ntStatus)) {
#ifdef DBGs
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - Could not start Bulk pipe failed 0x%x\n", ntStatus);
#endif
	}

	return ntStatus;
}


NTSTATUS
FakeMouse_EvtDeviceD0Exit(
IN  WDFDEVICE Device,
IN  WDF_POWER_DEVICE_STATE TargetState)
{
	PDEVICE_EXTENSION	devContext;
	NTSTATUS			ntStatus = STATUS_SUCCESS;

	PAGED_CODE();

#ifdef DBGs
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "=== Mouse Driver - FakeMouse_EvtDeviceD0Exit Enter- moving to %s\n", DbgDevicePowerString(TargetState));
#else
	UNREFERENCED_PARAMETER(TargetState);
#endif

	devContext = GetDeviceContext(Device);

#ifdef BULK
	WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(devContext->BulkINPipe), WdfIoTargetCancelSentIo);
#endif
#ifdef INTERRUPT
	WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(devContext->InterruptINPipe), WdfIoTargetCancelSentIo);
#endif

	return ntStatus;
}



PCHAR
DbgDevicePowerString(
IN WDF_POWER_DEVICE_STATE Type)
{
	switch (Type)
	{
	case WdfPowerDeviceInvalid:
		return "WdfPowerDeviceInvalid";
	case WdfPowerDeviceD0:
		return "WdfPowerDeviceD0";
	case WdfPowerDeviceD1:
		return "WdfPowerDeviceD1";
	case WdfPowerDeviceD2:
		return "WdfPowerDeviceD2";
	case WdfPowerDeviceD3:
		return "WdfPowerDeviceD3";
	case WdfPowerDeviceD3Final:
		return "WdfPowerDeviceD3Final";
	case WdfPowerDevicePrepareForHibernation:
		return "WdfPowerDevicePrepareForHibernation";
	case WdfPowerDeviceMaximum:
		return "WdfPowerDeviceMaximum";
	default:
		return "UnKnown Device Power State";
	}
}
