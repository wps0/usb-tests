#include <assert.h>
#include <stddef.h>
#include <libopencm3/stm32/st_usbfs.h>
// #include <libopencm3/usb/dwc/otg_fs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbd.h>
#include "usb.h"

static usbd_device *usbd_dev;

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
        .bcdHID = 0x0100,
        .bCountryCode = 0,
        .bNumDescriptors = 1
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor)
    }
};

/* 
 * Device descriptor - on top of it, at upper "layers",
 *  interface descriptors are defined.
 */
const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE, /* Descriptor type: DEVICE */
	.bcdUSB = 0x0200,
	.bDeviceClass = 0, /* TODO: nie powinien być kod klasy i subklasy HID > keyboard? */
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64, /* czy tu nie trzeba mniejszego? */
	.idVendor = 0x0483,
	.idProduct = 0x5710,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
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
     /* Descriptor type: INTERFACE */
	.bDescriptorType = USB_DT_INTERFACE,
    /* The position of this interface in the array of all interfaces for the device(?) */
	.bInterfaceNumber = 0,
	 /* TODO: ? */
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

const struct usb_config_descriptor cfg_descr = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 1,
	.bmAttributes = 0xC0, // nie sprawdzane co to znaczy! TODO: sprawdzic
	.bMaxPower = 2,
    .interface = ifaces
};


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

static uint8_t usbd_control_buffer[128];

// static enum usbd_request_return_codes simple_control_callback(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
// 		uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
// {
// 	(void)buf;
// 	(void)len;
// 	(void)complete;
// 	(void)usbd_dev;
// 	printf("usbd_request_return_codes!!\n");

// 	if (req->bmRequestType != 0x40)
// 		return USBD_REQ_NOTSUPP; /* Only accept vendor request. */

// 	/*if (req->wValue & 1)
// 		gpio_set(GPIOA, GPIO5);
// 	else
// 		gpio_clear(GPIOA, GPIO5);

// 	return USBD_REQ_HANDLED;
// }

// static void usb_set_config_cb(usbd_device *usbd_dev, uint16_t wValue)
// {
// 	printf("!!!!!!!!!!!!!!!!!!!!usb_set_config_cb!!\n");
// 	(void)wValue;
// 	usbd_register_control_callback(
// 				usbd_dev,
// 				USB_REQ_TYPE_VENDOR,
// 				USB_REQ_TYPE_TYPE,
// 				simple_control_callback);
// }

static void usb_out_cb(usbd_device *dev, uint8_t ep)
{
        (void)ep;

        char buf[1024];
        int len = usbd_ep_read_packet(dev, ep, buf, 128);


        for(int i = 0; i < len; i++)
            gpio_toggle(GPIOA, GPIO5);
			for (int i = 0; i < 0x200000; i++)
				__asm__("nop");
}

void usb_setup(void) {
    // _usb_gpio_setup();
    // 48MHz clock is required to init the USBD!
    printf("Creating USB device... ");
	// gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO12);
	// gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);
    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &cfg_descr, usb_strings, 3,
                        &usbd_control_buffer, sizeof(usbd_control_buffer));
	assert(usbd_dev != NULL);
    printf("OK\n");

	printf("Updating USB config callback... ");
    // usbd_register_set_config_callback(usbd_dev, usb_set_config_cb);
    printf("OK\n");
	
	// printf("usbd_dev address: 0x%X\n", usbd_dev);

    /* LED output */
    printf("Changing PA5 (GPIOA/GPIO5) mode... ");
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO5);
    // gpio_set_output_options()
    printf("OK\n");


	/* Delay some seconds to show that pull-up switch works. */
	printf("Delay some seconds to show that pull-up switch works...\n");
	for (int i = 0; i < 0x800000; i++)
		__asm__("nop");

	/* Wait for USB Vbus. */
	printf("Wait for USB Vbus...\n");
	// while (gpio_get(GPIOA, GPIO11) == 0 || gpio_get(GPIOA, GPIO12) == 0)
		// __asm__("nop");

	printf("Polling usbd_dev\n");
    while (1) {
		for (int i = 0; i < 128; i++)
			printf("%d", usbd_control_buffer[i]);
		printf("\n");
		usbd_poll(usbd_dev);
		// for (int i = 0; i < 0x200000; i++)
		// __asm__("nop");
    }
}