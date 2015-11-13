#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <minix/const.h>
#include <sys/ucred.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "secrets.h"

/*
 * Function prototypes for the hello driver.
 */
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_ioctl,      (struct driver *d, message *m) );
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
    secret_ioctl,
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
PRIVATE int secret_size;
/** Owner's name TODO: wut */
PRIVATE int owner;
/** How many processes are reading this secret right now? */
PRIVATE int num_reading_secret;
/** If the user didn't finish reading, save their spot till next time. */
PRIVATE int secret_posn;
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
    int res;
    struct ucred opening_user;

    res = getnucred(m->IO_ENDPT, &opening_user);
    if (res == -1) {
        perror("getnucred");
        return EACCES;
    }

    if ((m->COUNT & R_BIT) && (m->COUNT & W_BIT)) {
        /* Read-Write access not allowed!  What's the point
         *  of telling a secret to yourself?? */
        return EACCES;
    }

    printf("State: %d\n", device_state);

    if (m->COUNT & R_BIT) {
        printf("1\n");
        if (device_state == DEVICE_STATE_EMPTY) {
            /* No secrets here! Try again later. */
            return OK;
        } else if (device_state == DEVICE_STATE_BEING_WRITTEN) {
            printf("2\n");
            /* Somebody's writing a secret now.  Don't interrupt them,
             *  that would be RUDE. */
            return EACCES;
        }
        else if (device_state == DEVICE_STATE_FULL) {
            printf("Trying to read device. I am %d and owner is %d\n", opening_user.uid, owner);
            if (opening_user.uid == owner) {
                open_counter++;
                return OK;
            }
            else {
                /* That's not your secret! */
                return EACCES;
            }
        }
    }
    else if (m->COUNT & W_BIT) {
        printf("3\n");
        if (device_state != DEVICE_STATE_EMPTY) {
            printf("4\n");
            /** You can't write right now.  Wait 'till it's empty boss. */
            return ENOSPC;
        }

        owner = opening_user.uid;
    }

    printf("5\n");

    return OK;
}

PRIVATE int secret_close(d, m)
    struct driver *d;
    message *m;
{
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
         open_counter--;
         if (open_counter == 0) {
            /* Wipe secret */
            memset(secret, 0, SECRET_SIZE);
            secret_size = 0;
            device_state = DEVICE_STATE_EMPTY;
         }
    }

    return OK;
}

PRIVATE int secret_ioctl(d, m)
    struct driver *d;
    message *m;
{
    int res;
    struct ucred opening_user;
    uid_t grantee;

    res = getnucred(m->IO_ENDPT, &opening_user);
    if (res == -1) {
        perror("getnucred");
        return EACCES;
    }

    if (m->REQUEST != SSGRANT) {
        return ENOTTY;
    }

    if (opening_user.uid != owner) {
        return EACCES;
    }

    res = sys_safecopyfrom(m->IO_ENDPT, (vir_bytes)m->IO_GRANT,
                             0, (vir_bytes)&grantee, sizeof(grantee), D);

    owner = grantee;

    return res;
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

PRIVATE int do_read(proc_nr, iov, bytes)
    int proc_nr;
    iovec_t *iov;
    int bytes;
{
    int ret;

    if (device_state != DEVICE_STATE_FULL) {
        return OK;
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
        return EACCES;
    }

    /* Prevent user from writing past end of buffer */
    ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0, 
                          (vir_bytes) (secret),
                          iov->iov_size, D);
    iov->iov_size -= bytes;
    secret_posn = 0;

    device_state = DEVICE_STATE_BEING_WRITTEN;

    secret_size = iov->iov_size;

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


    if (bytes <= 0)
    {
        return OK;
    }
    switch (opcode)
    {
        case DEV_GATHER_S:
             ret = do_read(proc_nr, iov, bytes);
            break;
        case DEV_SCATTER_S:
            ret = do_write(proc_nr, iov, bytes);
            break;
        default:
            return EINVAL;
    }

    return ret;
}

PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
}

PRIVATE int sef_cb_lu_state_save(int state) {
    /* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);
    ds_publish_u32("secret_size", secret_size, DSF_OVERWRITE);
    ds_publish_u32("owner", owner, DSF_OVERWRITE);
    ds_publish_u32("num_reading_secret", num_reading_secret, DSF_OVERWRITE);
    ds_publish_u32("secret_posn", secret_posn, DSF_OVERWRITE);
    ds_publish_u32("device_state", device_state, DSF_OVERWRITE);

    return OK;
}

PRIVATE int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    ds_retrieve_u32("secret_size", &value);
    ds_delete_u32("secret_size");
    secret_size = (int) value;

    ds_retrieve_u32("owner", &value);
    ds_delete_u32("owner");
    owner = (int) value;

    ds_retrieve_u32("num_reading_secret", &value);
    ds_delete_u32("num_reading_secret");
    num_reading_secret = (int) value;

    ds_retrieve_u32("secret_posn", &value);
    ds_delete_u32("secret_posn");
    secret_posn = (int) value;

    ds_retrieve_u32("device_state", &value);
    ds_delete_u32("device_state");
    device_state = (int) value;

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
            /* INITIAL STARTUP STATE */
            do_announce_driver = FALSE;
            open_counter = 0;
            secret_size = 0;
            num_reading_secret = 0;
            secret_posn = 0;
            device_state = DEVICE_STATE_EMPTY;  
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();

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

