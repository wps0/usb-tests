#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <errno.h>
#include <libopencm3/stm32/st_usbfs.h>
// #include <libopencm3/usb/dwc/otg_fs.h>
#include <libopencm3/stm32/f3/memorymap.h>
#include <libopencm3/stm32/f3/gpio.h>
#include <libopencm3/stm32/f3/rcc.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/usbstd.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>

#include "usb.h"
#include "systick.h"


static const uint8_t hid_report_descriptor[] = {
	0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,        // USAGE (Keyboard)
	0xa1, 0x01,        // COLLECTION (Application)
	0x05, 0x07,        //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,        //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,        //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,        //   LOGICAL_MINIMUM (0)
	0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,        //   REPORT_SIZE (1)
	0x95, 0x08,        //   REPORT_COUNT (8)
	0x81, 0x02,        //   INPUT (Data,Var,Abs)
	0x95, 0x01,        //   REPORT_COUNT (1)
	0x75, 0x08,        //   REPORT_SIZE (8)
	0x81, 0x03,        //   INPUT (Cnst,Var,Abs)
	0x95, 0x05,        //   REPORT_COUNT (5)
	0x75, 0x01,        //   REPORT_SIZE (1)
	0x05, 0x08,        //   USAGE_PAGE (LEDs)
	0x19, 0x01,        //   USAGE_MINIMUM (Num Lock)
	0x29, 0x05,        //   USAGE_MAXIMUM (Kana)
	0x91, 0x02,        //   OUTPUT (Data,Var,Abs)
	0x95, 0x01,        //   REPORT_COUNT (1)
	0x75, 0x03,        //   REPORT_SIZE (3)
	0x91, 0x03,        //   OUTPUT (Cnst,Var,Abs)
	0x95, 0x06,        //   REPORT_COUNT (6)
	0x75, 0x08,        //   REPORT_SIZE (8)
	0x15, 0x00,        //   LOGICAL_MINIMUM (0)
	0x25, 0x65,        //   LOGICAL_MAXIMUM (101)
	0x05, 0x07,        //   USAGE_PAGE (Keyboard)
	0x19, 0x00,        //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,        //   USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,        //   INPUT (Data,Ary,Abs)
    0xC0    /*     END_COLLECTION	             */
};


static const struct {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(hid_function),
        .bDescriptorType = USB_DT_HID,
        /* Specifies which version of the HID specification this interface is compliant with */
        .bcdHID = 0x0200,
        .bCountryCode = 0,
        .bNumDescriptors = 1
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor)
    }
};

const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE, /* Descriptor type: DEVICE */
	/* The highest version of USB the device supports */
	.bcdUSB = 0x0200,
	/* If equal to Zero, each interface specifies it’s own class code */
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	/* Maximum Packet Size for Zero Endpoint. Valid Sizes are 8, 16, 32, 64 */
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	/* Device Release Number */
	.bcdDevice = 0x0200,
	/* Index of a string */
	.iManufacturer = 0,
	.iProduct = 1,
	.iSerialNumber = 2,
	.bNumConfigurations = 1, /* identifies the number of configurations the device supports */
};

const struct usb_endpoint_descriptor hid_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
    /**
     * Bit 3...0: The endpoint number
     * Bit 6...4: Reserved, reset to zero
     * Bit 7:
     *  Direction, ignored for control endpoints
     *  0 = OUT endpoint
     *  1 = IN endpoint
     */
	.bEndpointAddress = 0x81, /* TODO: w tym przykładzie jest IN endpoint, a nie OUT*/
    /* 0x03 - Transfer type: INTERRUPT */
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	/* In bytes */
    .wMaxPacketSize = 4,
    /* Low-level stuff */
	// .bInterval = 0x02,
	.bInterval = 0x20,
};

/*
 * HID Interface descriptor - an upper layer over the device descriptor.
 */
const struct usb_interface_descriptor hid_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
    /* The index of this interface in the array of all interfaces for the device. */
	.bInterfaceNumber = 0,
	 /* Indicates whether the interface can be an alternate interface to the one running
	  * on the address in bInterfaceNumber. When connected to a PC the host uses
	  * 0-alternate-setting interfaces but during operation the host can alternate
	  * between them. */
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
     /* Number of data endpoints (Default Control Pipe, id:0, excluded) */
	.bInterfaceClass = USB_CLASS_HID,
     /* The only unreserved subclass code in HID 1.1 specification is 1 (boot mode) */
	.bInterfaceSubClass = USB_HID_SUBCLASS_BOOT_INTERFACE,
    /* 1 (USB_HID_INTERFACE_PROTOCOL_KEYBOARD) - keyboard */
	.bInterfaceProtocol = USB_HID_INTERFACE_PROTOCOL_KEYBOARD,
	.iInterface = 0,

	/* Descriptor ends here.  The following are used internally: */
	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
}};

const struct usb_config_descriptor cfg_descr[1] = {{
    .bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	/* When the configuration descriptor is read, it returns the entire configuration
	 * hierarchy which includes all related interface and endpoint descriptors.
	 * The wTotalLength field reflects the number of bytes in the hierarchy. */
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	/* Value to use as an argument to select this configuration (SetConfiguration
	* request) */
	.bConfigurationValue = 0,
	/* The index to a string descriptor describing the configuration in the human readable
	 * form. */
	.iConfiguration = 0,
	/* Bit 7: Reserved, set to 1. (USB 1.0 Bus Powered)
	 * Bit 6: Self Powered
	 * Bit 5: Remote Wakeup
	 * Bit 4..0: Reserved, set to 0. */
	.bmAttributes = 0xC0,
	.bMaxPower = 0x32,
    .interface = ifaces
}};

// const struct usb_string_descriptor

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	"HID Demo",
	"DEMO",
};

// static void _usb_gpio_setup() {
    /* Setup GPIO pins for USB D+/D-. */
    // TODO: to tak zadziala? USB jest additional function, czyli funkcją
    //  włączaną przez jakiś tam inny register, a nie ten GPIO(?)
	// gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    
    // L2 setup
    // gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    // TODO: o co chodz z CNF_INPUT_FLOAT?
    // gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO11);
    
    // PA11 - DM, additional function
    // PA12 - DP, additional function
    // clock has to be enabled first (clock_setup function)
    // rcc_periph_clock_enable(RCC_)
// }


// static enum usbd_request_return_codes hid_control_request(usbd_devi)

static enum usbd_request_return_codes hid_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	printf("hid_control_request!!\n");
	(void)complete;
	(void)usbd_dev;

	// if((req->bmRequestType != 0x81) ||
	//    (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	//    (req->wValue != 0x2200))
	// 	return USBD_REQ_NOTSUPP;

	// if (req->wValue & 1)
		// gpio_set(GPIOA, GPIO5);
	// else
		// gpio_clear(GPIOA, GPIO5);
	
	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	return USBD_REQ_HANDLED;
}

static void usb_set_config_cb(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;
	printf("!!!!!!usb_set_config_cb!!\n");
	// usbd_register_control_callback(
	// 			usbd_dev,
	// 			USB_REQ_TYPE_VENDOR,
	// 			USB_REQ_TYPE_TYPE,
	// 			simple_control_callback);
	usbd_register_control_callback(usbd_dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);
	// https://github.com/libopencm3/libopencm3-examples/blob/fe6a2b5f13d1a938e6e1d5980b27682bd47407d2/examples/stm32/f1/other/usb_hid/usbhid.c systick?
}

// static void usb_out_cb(usbd_device *dev, uint8_t ep)
// {
//         (void)ep;

//         char buf[1024];
//         int len = usbd_ep_read_packet(dev, ep, buf, 128);


//         for(int i = 0; i < len; i++)
//             gpio_toggle(GPIOA, GPIO5);
// 			for (int i = 0; i < 0x200000; i++)
// 				__asm__("nop");
// }


void usb_lp_can1_rx0_isr(void) {
	// printf("USB RECEIVE\n");
}

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static void usb_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
	(void)ep;
	(void)usbd_dev;

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	if (len) {
		usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
		buf[len] = 0;
	}
	printf("len: %d\n", len);
}

void usb_setup(void) {
	usbd_device *usb_dev = 0;
    // _usb_gpio_setup();
    // 48MHz clock is required to init the USBD!
    printf("Creating USB device...\n");
	printf("Control buffer size: %d\n", sizeof(usbd_control_buffer));
	// gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    usb_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &cfg_descr[0], usb_strings, 3,
                        usbd_control_buffer, sizeof(usbd_control_buffer));
    printf("OK\n");

	printf("Updating USB config callback... ");
    int cfg_cb_status = usbd_register_set_config_callback(usb_dev, usb_set_config_cb);
	// struct usb_setup_data setup_data = {
	// 	.bmRequestType = 0x80,
	// 	.bRequest = 6,
	// 	.wValue = 0,
	// 	.wIndex = 0,
	// 	.wLength = 64,
	// };

	// usb_standard_set_configuration(usb_dev, &setup_data, usbd_control_buffer, sizeof(usbd_control_buffer));
	
	printf("CFG callback status: %d\n", cfg_cb_status);
	// usbd_ep_setup(usb_dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 64, usb_data_rx_cb);
    printf("OK\n");
	
    printf("Changing PC5 (GPIOC/GPIO5) mode...");
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
    printf("OK\n");


	/* Delay some seconds to show that pull-up switch works. */
	// printf("Delay some seconds to show that pull-up switch works...\n");
	
	printf("asd%d\n", gpio_get(GPIOC, GPIO5));
	
	
	gpio_set(GPIOC, GPIO5);

	/* Wait for USB Vbus. */
	printf("Wait for USB Vbus...\n");
	// while (gpio_get(GPIOC, GPIO5) == 0)
	// 	__asm__("nop");
	// printf("%d\n", gpio_get(GPIOC, GPIO5));


	
    while (1) {
		// for (int i = 0; i < 128; i++)
			// printf("%d", usbd_control_buffer[i]);
		// printf("\n");
		usbd_poll(usb_dev);
		// for (int i = 0; i < 0x200000; i++)
		// __asm__("nop");
		/* Standard Device requests without a data stage must be completed in 50ms. */
		delay(50);
	}
}


// static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
// 		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
// {
// 	(void)complete;
// 	(void)buf;
// 	(void)usbd_dev;

// 	switch (req->bRequest) {
// 	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
// 		/*
// 		 * This Linux cdc_acm driver requires this to be implemented
// 		 * even though it's optional in the CDC spec, and we don't
// 		 * advertise it in the ACM functional descriptor.
// 		 */
// 		char local_buf[10];
// 		struct usb_cdc_notification *notif = (void *)local_buf;

// 		/* We echo signals back to host as notification. */
// 		notif->bmRequestType = 0xA1;
// 		notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
// 		notif->wValue = 0;
// 		notif->wIndex = 0;
// 		notif->wLength = 2;
// 		local_buf[8] = req->wValue & 3;
// 		local_buf[9] = 0;
// 		// usbd_ep_write_packet(0x83, buf, 10);
// 		return USBD_REQ_HANDLED;
// 		}
// 	case USB_CDC_REQ_SET_LINE_CODING:
// 		if (*len < sizeof(struct usb_cdc_line_coding))
// 			return USBD_REQ_NOTSUPP;
// 		return USBD_REQ_HANDLED;
// 	}
// 	return USBD_REQ_NOTSUPP;
// }

// static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
// {
// 	(void)ep;
// 	(void)usbd_dev;

// 	char buf[64];
// 	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

// 	if (len) {
// 		usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
// 		buf[len] = 0;
// 	}
// }

// static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
// {
// 	(void)wValue;
// 	(void)usbd_dev;

// 	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
// 	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
// 	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

// 	usbd_register_control_callback(
// 				usbd_dev,
// 				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
// 				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
// 				cdcacm_control_request);
// }


static void _usb_setup(void)
{
	/* Enable clocks for GPIO port A and USB peripheral. */
	// rcc_periph_clock_enable(RCC_USB);
	// rcc_periph_clock_enable(RCC_GPIOA);

	/* Setup GPIO pins for USB D+/D-. */
	// gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	// gpio_set_af(GPIOA, GPIO_AF14, GPIO11 | GPIO12);
}

/*void usb_setup(void)
{
	printf("RCC clock setup...\n");
	//rcc_clock_setup_pll(&rcc_hse8mhz_configs[RCC_CLOCK_HSE8_72MHZ]);
	_usb_setup();

	printf("Initializing USB...");
	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &cfg_descr, usb_strings,
			3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
	printf("OK\nForcing the detection of new USB device...");
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	delay(1500);
	gpio_clear(GPIOC, GPIO5);
	delay(800);
	gpio_set(GPIOC, GPIO5);
	printf("OK\nPolling usbd_dev\n");
	while (1) {
		usbd_poll(usbd_dev);
	}
}*/