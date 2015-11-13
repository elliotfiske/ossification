#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/const.h>
#include "secrets.h"

/*
 * Function prototypes for the hello driver.
 */
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * secret_prepare, (int device) );
FORWARD _PROTOTYPE( int secret_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void secret_geometry, (struct partition *entry) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the hello driver. */
PRIVATE struct driver secret_tab =
{
    secret_name,
    secret_open,
    secret_close,
    nop_ioctl,
    secret_prepare,
    secret_transfer,
    nop_cleanup,
    secret_geometry,
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

/********************** DEVICE STATE *****************************/
/** State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;
/** How big a secret are we holding onto? */
PRIVATE unsigned long secret_size;
/** Owner's name TODO: wut */
PRIVATE char owner[1000];
/** How many processes are reading this secret right now? */
PRIVATE int num_reading_secret;
/** If the user didn't finish reading, save their spot till next time. */
PRIVATE unsigned long secret_posn;
/** State of the device driver. */
PRIVATE int device_state;


PRIVATE char * secret_name(void)
{
    printf("secret_name()\n");
    return "secret";
}

PRIVATE int secret_open(d, m)
    struct driver *d;
    message *m;
{
    if ((m->COUNT & R_BIT) && (m->COUNT & W_BIT)) {
        /* Read-Write access not allowed!  What's the point
         *  of telling a secret to yourself?? */
        return EACCES;
    }

    if (m->COUNT & R_BIT) {
        if (device_state == DEVICE_STATE_EMPTY) {
            /* No secrets here! Try again later. */
            return ENOENT;
        } else if (device_state == DEVICE_STATE_BEING_WRITTEN) {
            /* Somebody's writing a secret now.  Don't interrupt them,
             *  that would be RUDE. */
            return EACCES;
        }

        /* TODO: check if they are the owner of this secret, tell them NO if they're not */
    }
    else if (m->COUNT & W_BIT) {
        if (device_state != DEVICE_STATE_EMPTY) {
            /** You can't write right now, right?  Wait 'till it's empty boss. */
            return EACCES;
        }
    }

    printf("secret_open(). Called %d time(s). m_type: %d m_COUNT: %x\n", ++open_counter, m->m_type, m->COUNT);
    return OK;
}

PRIVATE int secret_close(d, m)
    struct driver *d;
    message *m;
{
    printf("secret_close()\n");

    /** If someone is in-progress writing to us and they
     *   close, we know it's safe to say we're full. */
    if (device_state == DEVICE_STATE_BEING_WRITTEN) {
        device_state = DEVICE_STATE_FULL;
    }
    else if (device_state == DEVICE_STATE_FULL) {
        /**
         * We're currently being read by some processes, and one closed.
         *  If there's nobody left reading us, it's safe to close and destroy the secret.
         */
         /* TODO: this */
    }

    return OK;
}

PRIVATE struct device * secret_prepare(dev)
    int dev;
{
    hello_device.dv_base.lo = 0;
    hello_device.dv_base.hi = 0;
    hello_device.dv_size.lo = SECRET_SIZE;
    hello_device.dv_size.hi = 0;
    return &hello_device;
}

int debug = 0;
void debug_printf(char *s) {
    if (debug) {
        printf("%s", s);
    }
}

PRIVATE int do_read(proc_nr, iov, bytes)
    int proc_nr;
    iovec_t *iov;
    int bytes;
{
    int ret;

    if (device_state != DEVICE_STATE_FULL) {
        return EACCES;
    }

    ret = sys_safecopyto(proc_nr, iov->iov_addr, 0,
                      (vir_bytes) (secret + secret_posn),
                       bytes, D);
    iov->iov_size -= bytes;
    secret_posn += bytes;

    return ret;
}

PRIVATE int do_write(proc_nr, iov, bytes)
    int proc_nr;
    iovec_t *iov;
    int bytes;
{
    int ret;

    if (device_state != DEVICE_STATE_EMPTY) {
        printf("Too bad! I'm full.\n");
        return EACCES;
    }

    /* Prevent user from writing past end of buffer */
    ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0, 
                          (vir_bytes) (secret),
                          iov->iov_size, D);
    iov->iov_size -= bytes;
    secret_posn = 0;

    device_state = DEVICE_STATE_BEING_WRITTEN;

    return ret;
}

PRIVATE int secret_transfer(proc_nr, opcode, position, iov, nr_req)
    int proc_nr;
    int opcode;
    u64_t position;
    iovec_t *iov;
    unsigned nr_req;
{
    int bytes, ret;

    bytes = SECRET_SIZE - secret_posn < iov->iov_size ?
             SECRET_SIZE - secret_posn : iov->iov_size;

    printf("Bytes = %d, strlen hello = %d, position.lo = %d, iov_size = %d, secret_posn = %d\n", bytes, strlen(SECRET_MESSAGE), position.lo, iov->iov_size, secret_posn);

    debug_printf("1\n");

    if (bytes <= 0)
    {
             printf("bad 2\n");
        return OK;
    }
    switch (opcode)
    {
        case DEV_GATHER_S:
             debug_printf("3\n");
             ret = do_read(proc_nr, iov, bytes);
            break;
        case DEV_SCATTER_S:
             debug_printf("4\n");
            ret = do_write(proc_nr, iov, bytes);
            break;
        default:
             debug_printf("5\n");
            return EINVAL;
    }

             debug_printf("6\n");
    return ret;
}

PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    printf("secret_geometry()\n");
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
/* Initialize the secret driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            device_state = DEVICE_STATE_EMPTY;  
            printf("STARTIN UP BUTTERCUP %s", SECRET_MESSAGE);
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

            printf("%sHey, I'm a new version!\n", SECRET_MESSAGE);
        break;

        case SEF_INIT_RESTART:
            device_state = DEVICE_STATE_EMPTY;  
            printf("%sHey, I've just been restarted!\n", SECRET_MESSAGE);
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
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}

