
#ifndef HID_IDS_IFEEL_H_FILE
#define HID_IDS_IFEEL_H_FILE


#define USB_IFEEL_BUZZ_IOCTL _IOW('U', 1, struct ifeel_command)

#define USB_IFEEL_MINOR_BASE 80
#define MAX_DEVICES 1

#define LOGITECH_VENDOR_ID            0x046d
#define LOGITECH_IFEEL_ID             0xc030

#define SAITEK_VENDOR_ID              0x06a3
#define SAITEK_TOUCH_FORCE_OPTICAL_ID 0xff05

#define BELKIN_VENDOR_ID              0x050d
#define BELKIN_NOSTROMO_N30_ID        0x0804

struct ifeel_command {
    unsigned int strength;
    unsigned int delay;
    unsigned int count;
};

#endif // HID_IDS_IFEEL_H_FILE
