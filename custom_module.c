

#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>	// copy_from_user 
#include <asm/io.h>		// IO Read/Write Functions
#include <linux/proc_fs.h>	// Proc File System Functions
#include <linux/seq_file.h>	// Sequence File Operations
#include <linux/ioport.h>	// release_mem_region
#include <linux/interrupt.h>	// interrupt 
#include <linux/irq.h>	
#include <linux/wait.h>
#include <linux/sched.h>


#define CHENILLARD_BASEADDR 0x76200000
#define REGION_SIZE 0xFFFF+1
#define SPEED_OFFSET 4
#define REGISTER_SIZE_TOTAL (3*(sizeof(unsigned int)))

#define BTNS_INT (91)
#define GIE_OFFSET (0x011C)
#define IER_OFFSET (0x0128)
#define ISR_OFFSET (0x120)

#define BUTTON_BASEADDR (0x41220000)

static int chenillard_major = 0;	/* dynamic by default */
static int button_major = 0;

module_param(chenillard_major, int, 0);
module_param(botton_major, int, 0);

typedef struct data_struct_s {
  unsigned int reg_0;
  unsigned int reg_1;
  unsigned int reg_2;
} data_struct_t;

MODULE_AUTHOR ("TELECOM Bretagne");
MODULE_LICENSE("Dual BSD/GPL");

// Virtual Base Address
void *chenillard_addr;
void *button_addr;

DECLARE_WAIT_QUEUE_HEAD(btns_wq);
//============================================================================
//  /dev/chenillard file operations
//  read : return the current settings (MODE,SPEED)
//  write: write set the settings (MODE,SPEED)
//============================================================================
int chenillard_i_open (struct inode *inode, struct file *filp)
{
  printk(KERN_INFO "chenillard: device opened.\n");
  return 0;
}

int chenillard_i_release (struct inode *inode, struct file *filp)
{ 
  printk(KERN_INFO "chenillard: device closed\n");
  return 0;
}

ssize_t chenillard_i_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
  printk(KERN_INFO "chenillard: device read\n");

  return copy_to_user(buf, chenillard_addr, count);
}

ssize_t chenillard_i_write (struct file *filp, const char __user *buf, size_t count,	loff_t *f_pos)
{
  data_struct_t data_struct;

  printk(KERN_INFO "chenillard: device write\n");

  if (count > REGISTER_SIZE_TOTAL) {
    printk(KERN_INFO "user cannot write more than %d bytes\n", REGISTER_SIZE_TOTAL);
  }
  if (copy_from_user(&data_struct, buf, count) != 0) {
    printk(KERN_INFO "user write failed!\n");
    return -1;
  }
  *(unsigned int *)chenillard_addr = data_struct.reg_0;
  *((unsigned int *)chenillard_addr + 1) = data_struct.reg_1;
  //*((unsigned int *)chenillard_addr + 2) = data_struct.reg_2;
  printk(KERN_INFO "reg0[%d], reg1[%d], reg2[%d]\n", data_struct.reg_0, data_struct.reg_1, data_struct.reg_2);
  

  return 0;
}

//button
int button_open (struct inode *inode, struct file *filp)
{
  printk(KERN_INFO "button: device opened.\n");
  return 0;
}

int button_release (struct inode *inode, struct file *filp)
{ 
  printk(KERN_INFO "button: device closed\n");
  return 0;
}

ssize_t button_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
  printk(KERN_INFO "button: device read\n");

  //return copy_to_user(buf, chenillard_addr, count);
}

struct file_operations chenillard_i_fops = {
	.owner	 = THIS_MODULE,
	.read	 = chenillard_i_read,
	.write	 = chenillard_i_write,
	.open	 = chenillard_i_open,
	.release = chenillard_i_release,
};

struct file_operations button_fops = {
	.owner	 = THIS_MODULE,
	.read    = button_read,
	.open	 = button_open,
	.release = button_release,
};

irqreturn_t (*handler)(int, void *, struct pt_regs *) {

}



//============================================================================
// chenillard_init: 
//  - setup /dev/chenillard device
//  - protect the memory region 
//  - io-remap the device BASE_ADDR
//  - setup led mode & speed 
//============================================================================
int chenillard_init(void)
{
  int result;
  struct resource *res = NULL;
 
  // register our device
  result = register_chrdev(chenillard_major, "chenillard", &chenillard_i_fops);
  if (result < 0) {
    printk(KERN_INFO "chenillard: can't get major number\n");
    return result;
  }
  // if the major number isn't fix, the major is dynamic 
  if (chenillard_major == 0) 
    chenillard_major = result;


  result = register_chrdev(button_major, "button", &button_fops);
  if (result < 0) {
    printk(KERN_INFO "button: can't get major number\n");
    return result;
  }
  // if the major number isn't fix, the major is dynamic 
  if (button_major == 0) 
    button_major = result;


  // protect the memory region 
  res = request_mem_region(CHENILLARD_BASEADDR, REGION_SIZE, "chenillardmem");
  if (res == NULL) {
    printk("chenillard memory pretction failed!\n");
    return -1;
  }

  res = request_mem_region(BUTTON_BASEADDR, REGION_SIZE, "button");
  if (res == NULL) {
    printk("button memory pretction failed!\n");
    return -1;
  }

  // remap the BASE_ADDR memory region
  chenillard_addr = ioremap_nocache(CHENILLARD_BASEADDR, REGION_SIZE);
  if (chenillard_addr == NULL) {
    printk("chenillard ioremap_nocache failed!");
    return -1;
  }
  // remap the BASE_ADDR memory region
  button_addr = ioremap_nocache(BUTTON_BASEADDR, REGION_SIZE);
  if (button_addr == NULL) {
    printk("button ioremap_nocache failed!");
    return -1;
  }

  // turn ON LED mode = 1
  *(unsigned int *)chenillard_addr = 0x1;

  // setup the default speed to 7
  *((unsigned int *)chenillard_addr + 1) = 0x7;

  irq_set_irq_type(BTNS_INT, IRQ_TYPE_EDGE_RISING);
  result = request_irq(BTNS_INT, btns_interrupt, 0, "btns", NULL);
  if (result) {
    printk(KERN_INFO "btn: can't get assigned irq\n");
  }

  //enable interrrupt
  iowrite32(1<<31, button_addr + GIE_OFFSET);
  iowrite32(1, button_addr + IER_OFFSET);
  return 0;
}





//============================================================================
// module_cleanup: 
//  - turn the led off 
//  - unregister the /dev/chenillard and /dev/btns
//  - unmap, and release memory region 
//============================================================================
void cleanup_custom_module(void)
{
  // turn OFF LED mode = 0 
  *(unsigned int *)chenillard_addr = 0x0;
  free_irq(BTNS_INT, NULL);
  
  // drop the /dev/chenillard entry 
  unregister_chrdev(chenillard_major, "chenillard");
  
  // iounmap 
  iounmap(chenillard_addr);
  
  // release protected memory 
  release_mem_region(CHENILLARD_BASEADDR, REGION_SIZE);

  printk(KERN_INFO "chenillard: module cleaned ...\n");
}


int init_custom_module(void)
{
  int result;

  printk(KERN_INFO "Custom Module init..\n");
  result = chenillard_init();
  if (result !=0) return result;

  printk(KERN_INFO "Custom Module:init done");
  return 0;
}



module_init(init_custom_module);
module_exit(cleanup_custom_module);
