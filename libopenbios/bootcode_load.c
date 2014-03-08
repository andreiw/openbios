/* 
 * Raw bootcode loader (CHRP/Apple %BOOT)
 * Written by Mark Cave-Ayland 2013
 */

#include "config.h"
#include "kernel/kernel.h"
#include "libopenbios/bindings.h"
#include "libopenbios/bootcode_load.h"
#include "libc/diskio.h"
#include "drivers/drivers.h"
#define printf printk
#define debug printk

int 
bootcode_load(ihandle_t dev)
{
    int retval = -1, count = 0, fd;
    unsigned long bootcode, loadbase, offset, loadsize, entry;

    /* Mark the saved-program-state as invalid */
    feval("0 state-valid !");

    fd = open_ih(dev);
    if (fd == -1) {
        goto out;
    }

    /* Default to loading at load-base */
    fword("load-base");
    loadbase = POP();
    
#ifdef CONFIG_PPC
    /*
     * Apple OF does not honor load-base and instead uses pmBootLoad
     * value from the boot partition descriptor.
     *
     * Tested with:
     *   a debian image with QUIK installed
     *   a debian image with iQUIK installed (https://github.com/andreiw/quik)
     *   an IQUIK boot floppy
     *   a NetBSD boot floppy (boots stage 2)
     */
    if (is_apple()) {
      feval("bootcode-base @");
      loadbase = POP();
      feval("bootcode-size @");
      loadsize = POP();
      feval("bootcode-entry @");
      entry = POP();

      printk("bootcode base 0x%lx, size 0x%lx, entry 0x%lx\n",
             loadbase, loadsize, entry);
    } else {
      entry = loadbase;

      /* Load as much as we can. */
      loadsize = 0;
    }
#endif
    
    bootcode = loadbase;
    offset = 0;
    
    if (loadsize) {
      if (seek_io(fd, offset) != -1)
        count = read_io(fd, (void *) bootcode, loadsize);
    } else {
      while(1) {
        if (seek_io(fd, offset) == -1)
          break;
        count = read_io(fd, (void *)bootcode, 512);
        offset += count;
        bootcode += count;
      }
    }

    /* If we didn't read anything then exit */
    if (!count) {
        goto out;
    }

    printk("entry = 0x%lx\n", entry);
    
    /* Initialise saved-program-state */
    PUSH(entry);
    feval("saved-program-state >sps.entry !");
    PUSH(offset);
    feval("saved-program-state >sps.file-size !");
    feval("bootcode saved-program-state >sps.file-type !");

    feval("-1 state-valid !");

out:
    close_io(fd);
    return retval;
}

