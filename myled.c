/*

    Copyright (C) 2017  Tomoya Igarashi

    This program is free software: you can redistribute it and/or modify

    it under the terms of the GNU General Public License as published by

    the Free Software Foundation, either version 3 of the License, or

    (at your option) any later version.

    This program is distributed in the hope that it will be useful,

    but WITHOUT ANY WARRANTY; without even the implied warranty of

    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License

    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <linux/module.h>

#include <linux/fs.h>

#include <linux/cdev.h>

#include <linux/device.h>

#include <asm/uaccess.h>

#include <linux/io.h>

#include <linux/sched.h>

MODULE_AUTHOR("Tomoya Igarashi");

MODULE_DESCRIPTION("driver for LED control");

MODULE_LICENSE("GPL");

MODULE_VERSION("0.1");

static dev_t dev;

static struct cdev cdv;

static struct class *cls = NULL;



static volatile u32 *gpio_base25 = NULL;

static volatile u32 *gpio_base24 = NULL;



static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){

	char c;
        int i;

	if(copy_from_user(&c,buf,sizeof(char)))

		return -EFAULT;

	if(c == '0'){

		gpio_base25[10] = 1 << 25;
                gpio_base24[10] = 1 << 24;

        }

	else if(c == '1'){

		gpio_base25[7] = 1 << 25;
                gpio_base24[7] = 1 << 24;

        }

        else if(c == '2'){

                gpio_base25[7] = 1 << 25;

        }

        else if(c == '3'){

                gpio_base24[7] = 1 << 24;

        }

        else if(c == '4'){

                for(i=0;i<10;i++){
                    gpio_base25[7] = 1 << 25;
                    gpio_base24[10] = 1 << 24;
                    int timeout1=jiffies+10;
                    while(jiffies<timeout1)
                        schedule();
                    gpio_base25[10] = 1 << 25;
                    gpio_base24[7] = 1 << 24;
                    int timeout2=jiffies+10;
                    while(jiffies<timeout2)
                        schedule();
                }
                gpio_base25[10] = 1 << 25;
                gpio_base24[10] = 1 << 24;
        }

        return 1;

}



static struct file_operations led_fops = {

	.owner = THIS_MODULE,

	.write = led_write

};



static int __init init_mod(void)

{

	int retval;



	gpio_base25 = ioremap_nocache(0x3f200000, 0xA0);



	const u32 led25 = 25;

	const u32 index25 = led25/10;

	const u32 shift25 = (led25%10)*3;

	const u32 mask25 = ~(0x7 << shift25);

	gpio_base25[index25] = (gpio_base25[index25] & mask25) | (0x1 << shift25);



        gpio_base24 = ioremap_nocache(0x3f200000, 0xA0);



        const u32 led24 = 24;

        const u32 index24 = led24/10;

        const u32 shift24 = (led24%10)*3;

        const u32 mask24 = ~(0x7 << shift24);

        gpio_base24[index24] = (gpio_base24[index24] & mask24) | (0x1 << shift24);



	retval =  alloc_chrdev_region(&dev, 0, 1, "myled");

	if(retval < 0){

		printk(KERN_ERR "alloc_chrdev_region failed.\n");

		return retval;

	}

	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));



	cdev_init(&cdv, &led_fops);

	cdv.owner = THIS_MODULE;

	retval = cdev_add(&cdv, dev, 1);

	if(retval < 0){

		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));

		return retval;

	}



	cls = class_create(THIS_MODULE,"myled");

	if(IS_ERR(cls)){

		printk(KERN_ERR "class_create failed.");

		return PTR_ERR(cls);

	}

	device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev));



	return 0;

}



static void __exit cleanup_mod(void)

{

	cdev_del(&cdv);

	device_destroy(cls, dev);

	class_destroy(cls);

	unregister_chrdev_region(dev, 1);

	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));

	iounmap(gpio_base25);
        iounmap(gpio_base24);

}



module_init(init_mod);

module_exit(cleanup_mod);

