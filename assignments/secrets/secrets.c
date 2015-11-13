#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "secrets.h"

/*
 * Function prototypes for the hello driver.
 */
FORWARD _PROTOTYPE( char * hello_name,   (void) );
FORWARD _PROTOTYPE( int hello_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int hello_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * hello_prepare, (int device) );
FORWARD _PROTOTYPE( int hello_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void hello_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the hello driver. */
PRIVATE struct driver hello_tab =
{
    hello_name,
    hello_open,
    hello_close,
    nop_ioctl,
    hello_prepare,
    hello_transfer,
    nop_cleanup,
    hello_geometry,
    nop_alarm,
    nop_cancel,
    nop_select,
    nop_ioctl,
    do_nop,
};

/** Represents the /dev/hello device. */
PRIVATE struct device hello_device;

/** Actual secret */
PRIVATE char secret[SECRET_SIZE];

/* DEVICE STATE */
/** State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;
/** How big a secret are we holding onto? */
PRIVATE unsigned long secret_size;
/** Owner's name TODO: wut */
PRIVATE char owner[1000];
/** If the user didn't finish reading, save their spot till next time. */
PRIVATE unsigned long secret_posn;


PRIVATE char * hello_name(void)
{
    printf("hello_name()\n");
    return "hello";
}

PRIVATE int hello_open(d, m)
    struct driver *d;
    message *m;
{
    printf("hello_open(). Called %d time(s).\n", ++open_counter);
    return OK;
}

PRIVATE int hello_close(d, m)
    struct driver *d;
    message *m;
{
    printf("hello_close()\n");
    return OK;
}

PRIVATE struct device * hello_prepare(dev)
    int dev;
{
    hello_device.dv_base.lo = 0;
    hello_device.dv_base.hi = 0;
    hello_device.dv_size.lo = strlen(HELLO_MESSAGE);
    hello_device.dv_size.hi = 0;
    return &hello_device;
}

int debug = 0;
void debug_printf(char *s) {
    if (debug) {
        printf("%s", s);
    }
}

PRIVATE int hello_transfer(proc_nr, opcode, position, iov, nr_req)
    int proc_nr;
    int opcode;
    u64_t position;
    iovec_t *iov;
    unsigned nr_req;
{
    int bytes, ret;
    size_t how_much_to_read;
    char buff[SECRET_SIZE];

    debug_printf("lel\n");
    /*sys_safecopyfrom(proc_nr, (vir_bytes) iov->iov_addr, 0, 
                      (vir_bytes) (buff),
                      iov->iov_size, D);*/
    debug_printf("lol\n");

    /*debug_printf("buff: %p, *buff: %s\n", buff, buff);*/

    /*printf("hello_transfer() opcode: %d, position: %d, iov_addr: %s, iov_size: %d\n", opcode, position, buff, iov->iov_size);*/
    printf("IOv size: %d\n", iov->iov_size);

    debug_printf("kek\n");

    bytes = strlen(HELLO_MESSAGE) - position.lo < iov->iov_size ?
             strlen(HELLO_MESSAGE) - position.lo : iov->iov_size;

             debug_printf("1\n");

    if (bytes <= 0)
    {
             debug_printf("bad 2\n");
        return OK;
    }
    switch (opcode)
    {
        case DEV_GATHER_S:
             debug_printf("3\n");
             how_much_to_read = bytes;
            if (how_much_to_read > SECRET_SIZE) {
                how_much_to_read = SECRET_SIZE;
            }
            printf("secret is being read and is: %s, bytes is %d\n", secret, bytes);
            ret = sys_safecopyto(proc_nr, iov->iov_addr, 0,
                              (vir_bytes) (HELLO_MESSAGE),
                               bytes, D);
            break;
        case DEV_SCATTER_S:
             debug_printf("4\n");
            
            /* Prevent user from writing past end of buffer */
            if (iov->iov_size < SECRET_SIZE) { 
                ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0, 
                                      (vir_bytes) (secret),
                                      iov->iov_size, D);
                printf("secret is now: %s\n", secret);
            }
            else {
                printf("Tried to write past end of buff\n");
                return OK;
            }
            break;

        default:

             debug_printf("5\n");
            return EINVAL;
    }

             debug_printf("6\n");
    return ret;
}

PRIVATE void hello_geometry(entry)
    struct partition *entry;
{
    printf("hello_geometry()\n");
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

PRIVATE int sef_cb_lu_state_save(int state) {
/* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);

    return OK;
}

PRIVATE int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    return OK;
}

PRIVATE void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

PRIVATE int sef_cb_init(int type, sef_init_info_t *info)
{
/* Initialize the hello driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            printf("%s", HELLO_MESSAGE);
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

            printf("%sHey, I'm a new version!\n", HELLO_MESSAGE);
        break;

        case SEF_INIT_RESTART:
            printf("%sHey, I've just been restarted!\n", HELLO_MESSAGE);
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        driver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

PUBLIC int main(int argc, char **argv)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    driver_task(&hello_tab, DRIVER_STD);
    return OK;
}

