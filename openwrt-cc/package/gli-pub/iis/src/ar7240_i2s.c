#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
////////////////////////
#include <linux/version.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
//#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>

#include "i2sio.h"
#include "ar7240.h"
#include "ar7240_i2s.h"		/* local definitions */



#include <linux/device.h>





#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>


#include <linux/sound.h>
#include <linux/soundcard.h>

#include <linux/pm.h>
#include <linux/clk.h>
#include <linux/platform_device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/dma.h>
#include <asm/dma-mapping.h>




#define GLZT_I2S_310KERNEL_CLASS 1

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
#define ar7240_dma_cache_sync(b, c)              \
        do {                                            \
                dma_cache_sync(NULL, (void *)b,         \
                                c, DMA_FROM_DEVICE);    \
        } while (0)

#define ar7240_cache_inv(d, s)                   \
        do {                                            \
        } while (0)
#define AR7240_IRQF_DISABLED     IRQF_DISABLED
#else
#define ar7240_dma_cache_sync(b, c)              \
        do {                                            \
                dma_cache_wback((unsigned long)b, c);   \
        } while (0)
#define ar7240_cache_inv(d, s)                   \
        do {                                            \
                dma_cache_inv((unsigned long)d, s);     \
        } while (0)
//#define AR7240_IRQF_DISABLED     SA_INTERRUPT
#endif


/* Keep the below in sync with WLAN driver code
   parameters */
#define AOW_PER_DESC_INTERVAL 125      /* In usec */
#define DESC_FREE_WAIT_BUFFER 200      /* In usec */



#define TRUE    1
#define FALSE   !(TRUE)
//start oss
#define AUDIO_FMT_MASK (AFMT_S16_LE)


//end oss



//start oss
static int audio_dev_dsp;
static int audio_dev_mixer;
static int audio_fmt;//��ʱδ��
static int audio_channels;//������
static int audio_rate;//����Ƶ��
static int wm8978_volume;//����
static int mixer_igain=0x4; /* -6db*///����������
static int audio_mix_modcnt;//����smdk2410_mixer_ioctl�Ĵ���




int ar7240_i2s_major = 14;
int ar7240_i2s_minor = 3;
int ar7240_i2s_num = 1;

#ifdef GLZT_I2S_310KERNEL_CLASS

struct cdev cdev;

#endif
static struct class *glzt_i2s_class;//�Զ������豸�ڵ�
static struct device	*glzt_i2s_class_dev;//�Զ������豸�ڵ�

//static int i2s_start;

module_param(ar7240_i2s_major, int, S_IRUGO);
module_param(ar7240_i2s_minor, int, S_IRUGO);

MODULE_AUTHOR("Jacob@Atheros");
MODULE_LICENSE("Dual BSD/GPL");


void ar7240_i2sound_i2slink_on(int master);
void ar7240_i2sound_request_dma_channel(int);
void ar7240_i2sound_dma_desc(unsigned long, int);
void ar7240_i2sound_dma_start(int);
void ar7240_i2sound_dma_pause(int);
void ar7240_i2sound_dma_resume(int);
void ar7240_i2s_clk(unsigned long, unsigned long);
int  ar7242_i2s_open(void); 
void ar7242_i2s_close(void);
irqreturn_t ar7240_i2s_intr(int irq, void *dev_id);


int ar7240_i2sound_dma_stopped(int);

/* 
 * XXX : This is the interface between i2s and wlan
 *       When adding info, here please make sure that
 *       it is reflected in the wlan side also
 */
typedef struct i2s_stats {
    unsigned int write_fail;
    unsigned int rx_underflow;
    unsigned int aow_sync_enabled;
    unsigned int sync_buf_full;
    unsigned int sync_buf_empty;
    unsigned int tasklet_count;
    unsigned int repeat_count;
} i2s_stats_t;    

i2s_stats_t stats;
//start oss
/*
static loff_t smdk2410_audio_llseek(struct file *file, loff_t offset,
				    int origin)
{
	return -ESPIPE;
}

static ssize_t smdk2410_audio_write(struct file *file, const char *buffer,
				    size_t count, loff_t * ppos)
{
	printk("smdk2410_audio_write\n");
}
static ssize_t smdk2410_audio_read(struct file *file, char *buffer,
				   size_t count, loff_t * ppos)
{
	printk("smdk2410_audio_read\n");
}
static unsigned int smdk2410_audio_poll(struct file *file,
					struct poll_table_struct *wait)
{
	printk("smdk2410_audio_poll\n");
}
static int smdk2410_audio_ioctl(struct inode *inode, struct file *file,
				uint cmd, ulong arg)
{
	printk("smdk2410_audio_ioctl\n");

}
static int smdk2410_audio_open(struct inode *inode, struct file *file)
{
	printk("smdk2410_audio_open\n");

}
static int smdk2410_audio_release(struct inode *inode, struct file *file)
{
	printk("smdk2410_audio_release\n");

}
*/

	void ar7240_gpio_setpin(unsigned int pin, unsigned int to)
	{
	
		//uint32_t r;
		//r = ar7240_reg_rd(MBOX_INT_STATUS);
		//ar7240_reg_rmw_clear(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));
	
		unsigned long flags;
		unsigned long dat;
		if(to)
		{
			local_irq_save(flags);
			
			//ar7240_reg_rmw_clear(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));//����Ϊ�ɶ�(����)״̬
			ar7240_reg_rmw_clear(AR7240_GPIO_OE,(IIS_CONTROL_CSB|IIS_CONTROL_SDIN|IIS_CONTROL_SCLK));
			dat = ar7240_reg_rd(AR7240_GPIO_IN);//��
	
			dat |= pin;   //��λ				   //�޸�
			//ar7240_reg_wr(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));
			ar7240_reg_wr(AR7240_GPIO_OE,(IIS_CONTROL_CSB|IIS_CONTROL_SDIN|IIS_CONTROL_SCLK));		 //����Ϊ��д(���)״̬
			ar7240_reg_wr(AR7240_GPIO_OUT,dat); //д
	
			local_irq_restore(flags);
		}
		else
		{
			local_irq_save(flags);
			
			//ar7240_reg_rmw_clear(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));//����Ϊ�ɶ�(����)״̬
			ar7240_reg_rmw_clear(AR7240_GPIO_OE,(IIS_CONTROL_CSB|IIS_CONTROL_SDIN|IIS_CONTROL_SCLK));
			dat = ar7240_reg_rd(AR7240_GPIO_IN);//��
	
			dat &= ~(pin); //���				   //�޸�
	
			//ar7240_reg_wr(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));		//����Ϊ��д(���)״̬
			ar7240_reg_wr(AR7240_GPIO_OE,(IIS_CONTROL_CSB|IIS_CONTROL_SDIN|IIS_CONTROL_SCLK));
			ar7240_reg_wr(AR7240_GPIO_OUT,dat); //д
	
			local_irq_restore(flags);
		}
	}
	



	static void wm8978_write_reg(unsigned char reg, unsigned int data)
	{
		int i;
		unsigned long flags;
		unsigned short val = (reg << 9) | (data & 0x1ff);
	
#if 1
		ar7240_gpio_setpin(IIS_CONTROL_CSB,1);
		ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
		ar7240_gpio_setpin(IIS_CONTROL_SCLK,1);
	
		local_irq_save(flags);
	
		for (i = 0; i < 16; i++){
			if (val & (1<<15))
			{
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,0);
				ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
				udelay(1);
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,1); 		
			}
			else
			{
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,0);
				ar7240_gpio_setpin(IIS_CONTROL_SDIN,0);
				udelay(1);
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,1); 		
			}
	
			val = val << 1;
		}
	
		ar7240_gpio_setpin(IIS_CONTROL_CSB,0);	
		udelay(1);
		ar7240_gpio_setpin(IIS_CONTROL_CSB,1);
		ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
		ar7240_gpio_setpin(IIS_CONTROL_SCLK,1);
	
#else
	
		ar7240_gpio_setpin(IIS_CONTROL_CSB,0);
		ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
		ar7240_gpio_setpin(IIS_CONTROL_SCLK,1);
	
		local_irq_save(flags);
	
		for (i = 0; i < 16; i++){
			if (val & (1<<15))
			{
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,0);
				ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
				udelay(1);
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,1); 		
			}
			else
			{
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,0);
				ar7240_gpio_setpin(IIS_CONTROL_SDIN,0);
				udelay(1);
				ar7240_gpio_setpin(IIS_CONTROL_SCLK,1); 		
			}
	
			val = val << 1;
		}
	
		ar7240_gpio_setpin(IIS_CONTROL_CSB,1);	
		udelay(1);
		ar7240_gpio_setpin(IIS_CONTROL_CSB,0);
		ar7240_gpio_setpin(IIS_CONTROL_SDIN,1);
		ar7240_gpio_setpin(IIS_CONTROL_SCLK,1);
	
	
#endif 
		
		local_irq_restore(flags);	
	}




static int smdk2410_mixer_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int ret;
	long val = 0;
	//mixer_info info;
	//_old_mixer_info info;

	switch (cmd) {
		case SOUND_MIXER_INFO://����Ӳ����Ϣ
		{
			printk("cmd:SOUND_MIXER_INFO\n");
			//mixer_info info;
			/*
			strncpy(info.id, "WM8978", sizeof(info.id));
			strncpy(info.name,"Wolfson WM8978", sizeof(info.name));
			info.modify_counter = audio_mix_modcnt;
			return copy_to_user((void *)arg, &info, sizeof(info));
			*/
			break;
		}

		case SOUND_OLD_MIXER_INFO://�����ϵ�Ӳ����Ϣ
		{
			printk("cmd:SOUND_OLD_MIXER_INFO\n");
			//_old_mixer_info info;
			/*
			strncpy(info.id, "WM8978", sizeof(info.id));
			strncpy(info.name,"Wolfson WM8978", sizeof(info.name));
			return copy_to_user((void *)arg, &info, sizeof(info));
			*/
			break;
		}

		case SOUND_MIXER_READ_STEREODEVS://ֱ�ӷ���0
			printk("cmd:SOUND_MIXER_READ_STEREODEVS\n");
			return put_user(0, (long *) arg);

		case SOUND_MIXER_READ_CAPS://һ��ֻ��һ����¼��Դ,��֪���Ǹ���ģ������Ƿ���1
			printk("cmd:SOUND_MIXER_READ_CAPS\n");
			val = SOUND_CAP_EXCL_INPUT;
			return put_user(val, (long *) arg);

		case SOUND_MIXER_WRITE_VOLUME://������������оƬ��������
			printk("cmd:SOUND_MIXER_WRITE_VOLUME\n");
			ret = get_user(val, (long *) arg);
			if (ret)
				return ret;
			//uda1341_volume = 63 - (((val & 0xff) + 1) * 63) / 100;
			//uda1341_l3_address(UDA1341_REG_DATA0);
			//uda1341_l3_data(uda1341_volume);
			wm8978_volume = ((val & 0xff)*32)/100; 
			wm8978_write_reg(52,((1<<8)|wm8978_volume));
			wm8978_write_reg(53,((1<<8)|wm8978_volume));
			break;

		case SOUND_MIXER_READ_VOLUME://��ȡ���ڵ�����
			printk("cmd:SOUND_MIXER_READ_VOLUME\n");
			//val = ((63 - uda1341_volume) * 100) / 63;
			//val |= val << 8;
			val = (wm8978_volume * 100)/63;
			return put_user(val, (long *) arg);

		case SOUND_MIXER_READ_IGAIN://��ȡ����
			printk("cmd:SOUND_MIXER_READ_IGAIN\n");
			val = ((31- mixer_igain) * 100) / 31;
			return put_user(val, (int *) arg);

		case SOUND_MIXER_WRITE_IGAIN://�趨��������
			printk("cmd:SOUND_MIXER_WRITE_IGAIN\n");
			ret = get_user(val, (int *) arg);
			if (ret)
				return ret;
			mixer_igain = 31 - (val * 31 / 100);
			/* use mixer gain channel 1*/
			//uda1341_l3_address(UDA1341_REG_DATA0);
			//uda1341_l3_data(EXTADDR(EXT0));
			//uda1341_l3_data(EXTDATA(EXT0_CH1_GAIN(mixer_igain)));
			break;

		default:
			printk("mixer ioctl %u unknown\n", cmd);
			return -ENOSYS;
	}

	audio_mix_modcnt++;//����smdk2410_mixer_ioctl�Ĵ���
	return 0;
}

static int smdk2410_mixer_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int smdk2410_mixer_release(struct inode *inode, struct file *file)
{
	return 0;
}


//end oss

/*
static struct file_operations smdk2410_audio_fops = {
	llseek: smdk2410_audio_llseek,
	write: smdk2410_audio_write,
	read: smdk2410_audio_read,
	poll: smdk2410_audio_poll,
	ioctl: smdk2410_audio_ioctl,
	open: smdk2410_audio_open,
	release: smdk2410_audio_release
};
*/
static struct file_operations smdk2410_mixer_fops = {
	.unlocked_ioctl = smdk2410_mixer_ioctl,
	.open = smdk2410_mixer_open,
	.release = smdk2410_mixer_release
};
//end oss



/*
 * Functions and data structures relating to the Audio Sync feature
 *
 */

int ar7240_i2sound_dma_stopped(int);


typedef struct i2s_buf {
	uint8_t *bf_vaddr;
	unsigned long bf_paddr;
} i2s_buf_t;


typedef struct i2s_dma_buf {
	ar7240_mbox_dma_desc *lastbuf;//δ��
	ar7240_mbox_dma_desc *db_desc;
	dma_addr_t db_desc_p;//�����ַ��typedef u32 dma_addr_t;
	i2s_buf_t db_buf[NUM_DESC];
	int tail;
} i2s_dma_buf_t;

typedef struct ar7240_i2s_softc {
	int ropened;//���򿪱�־
	int popened;//д�򿪱�־
	i2s_dma_buf_t sc_pbuf;//kmalloc�ڴ�������¼������ʱ���õ�ָ��
	i2s_dma_buf_t sc_rbuf;//kmalloc�ڴ������ŷ�������ʱ���õ�ָ��
	char *sc_pmall_buf;//kfreeʱ�������ͷ�¼�����ݵ��ڴ�
	char *sc_rmall_buf;//kfreeʱ�������ͷŷ������ݵ��ڴ�
	int sc_irq;
	int ft_value;
	int ppause;//д��ͣ��־
	int rpause;//����ͣ��־
	spinlock_t i2s_lock;   /* lock������ */
	unsigned long i2s_lockflags;
} ar7240_i2s_softc_t;


ar7240_i2s_softc_t sc_buf_var;
i2s_stats_t stats;


static void init_wm8978(void)
{
	//uda1341_volume = 57;
	//uda1341_boost = 0;

	/* software reset */
	wm8978_write_reg(0, 0);

	wm8978_write_reg(0x3, 0x6f);
	
	wm8978_write_reg(0x1, 0x1f);//biasen,BUFIOEN.VMIDSEL=11b  
	wm8978_write_reg(0x2, 0x185);//ROUT1EN LOUT1EN, inpu PGA enable ,ADC enable

	wm8978_write_reg(0x6, 0x0);//SYSCLK=MCLK  
	wm8978_write_reg(0x4, 0x10);//16bit 		
	wm8978_write_reg(0x2B,0x10);//BTL OUTPUT  
	wm8978_write_reg(0x9, 0x50);//Jack detect enable  
	wm8978_write_reg(0xD, 0x21);//Jack detect  
	wm8978_write_reg(0x7, 0x01);//Jack detect 



}


void i2s_get_stats(i2s_stats_t *p)
{
    memcpy(p, &stats, sizeof(struct i2s_stats));
}    

void i2s_clear_stats(void)
{
   stats.write_fail = 0; 
   stats.rx_underflow = 0;
   stats.sync_buf_full = 0;
   stats.sync_buf_empty = 0;
   stats.tasklet_count = 0;
   stats.repeat_count = 0;
}



int ar7240_i2s_init(struct file *filp)
{
	ar7240_i2s_softc_t *sc = &sc_buf_var;
	i2s_dma_buf_t *dmabuf;
	i2s_buf_t *scbuf;
	uint8_t *bufp = NULL;
	int j, byte_cnt, tail = 0, mode = 1;
	ar7240_mbox_dma_desc *desc;
	unsigned long desc_p;
	//���ô򿪱�־
    if (!filp) {
        mode = FMODE_WRITE;
    } else {
        mode = filp->f_mode;
    }

	if (mode & FMODE_READ) {
		dmabuf = &sc->sc_rbuf;
		sc->ropened = 1;
		sc->rpause = 0;
	} else {
		dmabuf = &sc->sc_pbuf;
		sc->popened = 1;
		sc->ppause = 0;
	}
	//DMA�Ĳ�������Ҫ�����ַ�ģ�������linux�ں���ʹ�õĶ��������ַ�����������Ҫ	
	//��DMA��һ���ڴ���в�����������εõ���һ���ڴ�������ַ�������ַ��ӳ��	
	//dma_alloc_coherent�������ʵ�������ֻ���	
	//dmabuf->db_desc�ڴ��������ʼ��ַ�����ں�Ҫ�ô˵�ַ��������������ڴ�	
	//dmabuf->db_desc_p����������ڴ��ʵ�������ַ��DMA��ʹ�ã�dmabuf->db_desc��	
	//��dmabuf->db_desc_p��Ӧ�������ַ������ϵͳ����
	dmabuf->db_desc = (ar7240_mbox_dma_desc *)dma_alloc_coherent(NULL,
				       NUM_DESC *sizeof(ar7240_mbox_dma_desc),
				       &dmabuf->db_desc_p, GFP_DMA);

	if (dmabuf->db_desc == NULL) {
		printk(KERN_CRIT "DMA desc alloc failed for %d\n",mode);
			return ENOMEM;
	}

	for (j = 0; j < NUM_DESC; j++) {
		dmabuf->db_desc[j].OWN = 0;
		}

		/* Allocate data buffers */
		scbuf = dmabuf->db_buf;

		if (!(bufp = kmalloc(NUM_DESC * I2S_BUF_SIZE, GFP_KERNEL))) {
			printk(KERN_CRIT"Buffer allocation failed for \n");
			goto fail3;
		}

	if (mode & FMODE_READ)
			sc->sc_rmall_buf = bufp;
		else
			sc->sc_pmall_buf = bufp;
		//ӳ��һ�鴦�����������ַ������������������ʣ��ú��������ڴ�������ַ
		for (j = 0; j < NUM_DESC; j++) {
			scbuf[j].bf_vaddr = &bufp[j * I2S_BUF_SIZE];
			scbuf[j].bf_paddr =dma_map_single(NULL, scbuf[j].bf_vaddr,I2S_BUF_SIZE,DMA_BIDIRECTIONAL);
		}
		dmabuf->tail = 0;

		// Initialize desc
			desc = dmabuf->db_desc;
			desc_p = (unsigned long) dmabuf->db_desc_p;
			byte_cnt = NUM_DESC * I2S_BUF_SIZE;
			tail = dmabuf->tail;

			while (byte_cnt && (tail < NUM_DESC)) {
				desc[tail].rsvd1 = 0;
				desc[tail].size = I2S_BUF_SIZE;
				if (byte_cnt > I2S_BUF_SIZE) {
					desc[tail].length = I2S_BUF_SIZE;
					byte_cnt -= I2S_BUF_SIZE;
					desc[tail].EOM = 0;
				} else {
					desc[tail].length = byte_cnt;
					byte_cnt = 0;
					desc[tail].EOM = 0;
				}
				desc[tail].rsvd2 = 0;
				desc[tail].rsvd3 = 0;
				desc[tail].BufPtr =(unsigned int) scbuf[tail].bf_paddr;
				desc[tail].NextPtr =(desc_p +((tail +1) *(sizeof(ar7240_mbox_dma_desc))));
				if (mode & FMODE_READ) {
					desc[tail].OWN = 1;
				} else {
					desc[tail].OWN = 0;
				}
				tail++;
			}
			tail--;
			desc[tail].NextPtr = desc_p;

		dmabuf->tail = 0;
	return 0;

fail3:
	if (mode & FMODE_READ)
			dmabuf = &sc->sc_rbuf;
		else
			dmabuf = &sc->sc_pbuf;
		dma_free_coherent(NULL,
				  NUM_DESC * sizeof(ar7240_mbox_dma_desc),
				  dmabuf->db_desc, dmabuf->db_desc_p);
		//��δִ�е�Ϊsc->sc_rmall_buf�����ڴ棬Ϊʲô����Ҫ�ͷ�????
	if (mode & FMODE_READ) {
			if (sc->sc_rmall_buf)
				kfree(sc->sc_rmall_buf);
		} else {
			if (sc->sc_pmall_buf)
				kfree(sc->sc_pmall_buf);
		}

	return -ENOMEM;

}


//���ļ�
int ar7240_i2s_open(struct inode *inode, struct file *filp)
{

	ar7240_i2s_softc_t *sc = &sc_buf_var;
	int opened = 0, mode = MASTER;


	if ((filp->f_mode & FMODE_READ) && (sc->ropened)) {
        printk("%s, %d I2S mic busy\n", __func__, __LINE__);
		return -EBUSY;
	}
	if ((filp->f_mode & FMODE_WRITE) && (sc->popened)) {
        printk("%s, %d I2S speaker busy\n", __func__, __LINE__);
		return -EBUSY;
	}

	opened = (sc->ropened | sc->popened);//����Ӧ�ö���0

	/* Reset MBOX FIFO's */
	if (!opened) {
		//2bit/0bit:Writing a 1 causes a RX/TX FIFO reset,		
		//The register is automatically reset to 0, and will always return 0 on a read		
		//����DMA
		ar7240_reg_wr(MBOX_FIFO_RESET, 0xff); // virian
		udelay(500);
	}

	/* Allocate and initialize descriptors */
	if (ar7240_i2s_init(filp) == ENOMEM)
		return -ENOMEM;

	if (!opened) {
	    ar7240_i2sound_request_dma_channel(mode);
    }

	return (0);

}


ssize_t ar7240_i2s_read(struct file * filp, char __user * buf,size_t count, loff_t * f_pos)//gl-inet
{
#define prev_tail(t) ({ (t == 0) ? (NUM_DESC - 1) : (t - 1); })
#define next_tail(t) ({ (t == (NUM_DESC - 1)) ? 0 : (t + 1); })

	uint8_t *data;
	//ssize_t retval;
	unsigned long retval;
	struct ar7240_i2s_softc *sc = &sc_buf_var;
	i2s_dma_buf_t *dmabuf = &sc->sc_rbuf;
	i2s_buf_t *scbuf;
	ar7240_mbox_dma_desc *desc;
	unsigned int byte_cnt, mode = 1, offset = 0, tail = dmabuf->tail;
	unsigned long desc_p;
	int need_start = 0;

	byte_cnt = count;

	if (sc->ropened < 2) {
		ar7240_reg_rmw_set(MBOX_INT_ENABLE, MBOX0_TX_DMA_COMPLETE);
		need_start = 1;
	}

	sc->ropened = 2;

	scbuf = dmabuf->db_buf;
	desc = dmabuf->db_desc;
	desc_p = (unsigned long) dmabuf->db_desc_p;
	data = scbuf[0].bf_vaddr;

	desc_p += tail * sizeof(ar7240_mbox_dma_desc);

	while (byte_cnt && !desc[tail].OWN) {
		if (byte_cnt >= I2S_BUF_SIZE) {
			desc[tail].length = I2S_BUF_SIZE;
			byte_cnt -= I2S_BUF_SIZE;
		} else {
			desc[tail].length = byte_cnt;
			byte_cnt = 0;
		}
		//ar7240_dma_cache_sync(scbuf[tail].bf_vaddr, desc[tail].length);//gl-inet

		dma_cache_sync(NULL, scbuf[tail].bf_vaddr, desc[tail].length, DMA_FROM_DEVICE);//gl-inet		
		desc[tail].rsvd2 = 0;//gl-inet
		
		retval = copy_to_user((buf + offset), (scbuf[tail].bf_vaddr), I2S_BUF_SIZE);

		if (retval)
			return retval;
		desc[tail].BufPtr = (unsigned int) scbuf[tail].bf_paddr;
		desc[tail].OWN = 1;

		tail = next_tail(tail);
		offset += I2S_BUF_SIZE;
	}

	dmabuf->tail = tail;

	if (need_start) {
		ar7240_i2sound_dma_desc((unsigned long) desc_p, mode);
        if (filp) {
		    ar7240_i2sound_dma_start(mode);
        }
	} else if (!sc->rpause) {
		ar7240_i2sound_dma_resume(mode);
	}

	return offset;
}


ssize_t ar7240_i2s_write(struct file * filp, const char __user * buf,
			 size_t count, loff_t * f_pos, int resume)
{
#define prev_tail(t) ({ (t == 0) ? (NUM_DESC - 1) : (t - 1); })
#define next_tail(t) ({ (t == (NUM_DESC - 1)) ? 0 : (t + 1); })

	uint8_t *data;
	ssize_t retval;
	int byte_cnt, offset, need_start = 0;
	int mode = 0;
	struct ar7240_i2s_softc *sc = &sc_buf_var;
	i2s_dma_buf_t *dmabuf = &sc->sc_pbuf;
	i2s_buf_t *scbuf;
	ar7240_mbox_dma_desc *desc;
	int tail = dmabuf->tail;
	unsigned long desc_p;
    int data_len = 0;


	I2S_LOCK(sc);

	byte_cnt = count;

	if (sc->popened < 2) {
        ar7240_reg_rmw_set(MBOX_INT_ENABLE, MBOX0_RX_DMA_COMPLETE | RX_UNDERFLOW);
		need_start = 1;
	}

	sc->popened = 2;

	scbuf = dmabuf->db_buf;
	desc = dmabuf->db_desc;
	desc_p = (unsigned long) dmabuf->db_desc_p;
	offset = 0;
	data = scbuf[0].bf_vaddr;

	desc_p += tail * sizeof(ar7240_mbox_dma_desc);

	while (byte_cnt && !desc[tail].OWN) {
        if (byte_cnt >= I2S_BUF_SIZE) {
			desc[tail].length = I2S_BUF_SIZE;
			byte_cnt -= I2S_BUF_SIZE;
            data_len = I2S_BUF_SIZE;
		} else {
			desc[tail].length = byte_cnt;
            data_len = byte_cnt;
			byte_cnt = 0;
		}

        if(!filp)
        {
            memcpy(scbuf[tail].bf_vaddr, buf + offset, data_len);
        }
        else
        {
            retval = copy_from_user(scbuf[tail].bf_vaddr, buf + offset, data_len);
            if (retval)
                return retval;
        }

        dma_cache_sync(NULL, scbuf[tail].bf_vaddr, desc[tail].length, DMA_TO_DEVICE);
		desc[tail].BufPtr = (unsigned int) scbuf[tail].bf_paddr;
		desc[tail].OWN = 1;
		tail = next_tail(tail);
		offset += data_len;
	}

	dmabuf->tail = tail;

	if (need_start) {
		ar7240_i2sound_dma_desc((unsigned long) desc_p, mode);
		ar7240_i2sound_dma_start(mode);
	}
	else if (!sc->ppause) {
		ar7240_i2sound_dma_resume(mode);
	}

//    if (resume)
//        ar7240_i2sound_dma_resume(mode);

	I2S_UNLOCK(sc);

	return count - byte_cnt;
}




ssize_t ar9100_i2s_write(struct file * filp, const char __user * buf,size_t count, loff_t * f_pos)
{
    int tmpcount, ret = 0;
    int cnt = 0;
//    char *data;
	

eagain:
    tmpcount = count;
   // data = buf;
    ret = 0;


    do {
        ret = ar7240_i2s_write(filp, buf, tmpcount, f_pos, 1);
        cnt++;
        if (ret == EAGAIN) {
            printk("%s:%d %d\n", __func__, __LINE__, ret);
            goto eagain;
        }

        tmpcount = tmpcount - ret;
        buf += ret;
    } while(tmpcount > 0);

    return count;
}



//�ر��ļ�
int ar7240_i2s_close(struct inode *inode, struct file *filp)
{
	int j, own, mode;
	ar7240_i2s_softc_t *sc = &sc_buf_var;
	i2s_dma_buf_t *dmabuf;
	ar7240_mbox_dma_desc *desc;
    int status = TRUE;
    int own_count = 0;

    if (!filp) {
        mode  = 0;
    } else {
        mode = filp->f_mode;
    }

	if (mode & FMODE_READ) {
		dmabuf = &sc->sc_rbuf;
		own = sc->rpause;
	} else {
		dmabuf = &sc->sc_pbuf;
		own = sc->ppause;
	}

	desc = dmabuf->db_desc;
	//�ж�ʱ��Ϊ��ͣ״̬�������ֱ�����¿�ʼ�����������ȴ���Դ�ͷ�
	if (own) {
		for (j = 0; j < NUM_DESC; j++) {
			desc[j].OWN = 0;
		}
		ar7240_i2sound_dma_resume(mode);
    } else {
        for (j = 0; j < NUM_DESC; j++) {
            if (desc[j].OWN) {
                own_count++;
            }
        }
        
        /* 
         * The schedule_timeout_interruptible is commented
         * as this function is called from other process
         * context, i.e. that of wlan device driver context
         * schedule_timeout_interruptible(HZ);
         */
	//�ȴ���Դ�ͷ�
        if (own_count > 0) { 
            udelay((own_count * AOW_PER_DESC_INTERVAL) + DESC_FREE_WAIT_BUFFER);
            
            for (j = 0; j < NUM_DESC; j++) {
                /* break if the descriptor is still not free*/
                if (desc[j].OWN) {
                    status = FALSE;
                    printk("I2S : Fatal error\n");
                    break;
                }
            }
        }
    }
	//������
	for (j = 0; j < NUM_DESC; j++) {
		dma_unmap_single(NULL, dmabuf->db_buf[j].bf_paddr,
				 I2S_BUF_SIZE, DMA_BIDIRECTIONAL);
	}

	if (mode & FMODE_READ)
		kfree(sc->sc_rmall_buf);
	else
		kfree(sc->sc_pmall_buf);
	dma_free_coherent(NULL,
			  NUM_DESC * sizeof(ar7240_mbox_dma_desc),
			  dmabuf->db_desc, dmabuf->db_desc_p);

	if (mode & FMODE_READ) {
		sc->ropened = 0;
		sc->rpause = 0;
	} else {
		sc->popened = 0;
		sc->ppause = 0;
	}

	return (status);
}

int ar7240_i2s_release(struct inode *inode, struct file *filp)
{
	printk(KERN_CRIT "release\n");
	return 0;
}





static long ar7240_i2s_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{ 
	//����Ĭ�ϵ�ʱ�ӼĴ�������
	//�໥�����ļĴ���ֱ�ӵĹ�ϵ
	//AR7240_STEREO_CONFIG_PCM_SWAP�ǣ�����֮ǰ��Դ����������ֻ��DATA_WORD_SIZE���ڵ���16ʱ��ʹ��
	//AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_XXB)�������������DATA_WORD_SIZE��
	//AR7240_STEREO_CONFIG_I2S_32B_WORD,���ֻ�������ˣ�����32λ����ȻĬ�Ͼ���16λ
	//AR7240_STEREO_CONFIG_PSEDGE(x),�������I2S_32B_WORDӰ�죬������BITCLK=MCLK/x*4,����Ϊ32λ��BITCLK=MCLK/x*2
	//����������AR7240_STEREO_CONFIG_PSEDGE�ǹ̶��ģ���Ϊ���I2S_32B_WORD��32���64�������BITCLK->WS�ĳ���������2��
	//����MCLK->BITCLK��ϵ��ҲӦ����С2��
#define AR7240_STEREO_CONFIG_DEFAULT (AR7240_STEREO_CONFIG_SPDIF_ENABLE| \
                AR7240_STEREO_CONFIG_ENABLE| \
                AR7240_STEREO_CONFIG_RESET| \
                AR7240_STEREO_CONFIG_MIC_WORD_SIZE| \
                AR7240_STEREO_CONFIG_MODE(0)| \
                 AR7240_STEREO_CONFIG_PSEDGE(2) | \
                AR7240_STEREO_CONFIG_SAMPLE_CNT_CLEAR_TYPE| \
                AR7240_STEREO_CONFIG_MASTER) //gl-inet       

	int data, mask = 0, cab = 0, cab1 = 0, j, st_cfg = 0;
	long val;
	struct ar7240_i2s_softc *sc = &sc_buf_var;
	i2s_dma_buf_t *dmabuf;

    	if (filp->f_mode & FMODE_READ) {
        	dmabuf = &sc->sc_rbuf;
    	} else {
        	dmabuf = &sc->sc_pbuf;
		}

//start oss
//����Ĭ��λ���������������Ӧ�ó�����Ƶ�ioctl���õģ�����ossģ����û�����ã�����������һ��Ĭ��ֵ
#if 1
					//case 8:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_8B)
						  		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16);
#endif
#if 0
				
					//case 16:
            			st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
            			  		AR7240_STEREO_CONFIG_PCM_SWAP |
                 		  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B)
                 		  		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16);
#endif
#if 0         			
				
					//case 24:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
                		  		AR7240_STEREO_CONFIG_PCM_SWAP |
                		  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_24B) |
                 		  		AR7240_STEREO_CONFIG_I2S_32B_WORD
                 		  		);
           				cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32);
#endif
#if 0            			
					//case 32:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
                		  		AR7240_STEREO_CONFIG_PCM_SWAP |
                	      		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_32B) |
                 	      		AR7240_STEREO_CONFIG_I2S_32B_WORD
                 	      		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32);
            			

						
        		
        		
#endif

	ar7240_reg_wr(AR7240_STEREO_CONFIG, (st_cfg | AR7240_STEREO_CONFIG_RESET));
    udelay(100);
	ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG,AR7240_STEREO_CONFIG_RESET);//�������λλ����Ȼʱ��ֹͣ���
    ar7240_reg_wr(AR7240_STEREO_CONFIG, st_cfg);
    for (j = 0; j < NUM_DESC; j++) {
    	dmabuf->db_desc[j].Ca[1] |= cab1;
        dmabuf->db_desc[j].Cb[1] |= cab1;
    }


//����Ĭ�������������������Ӧ�ó�����Ƶ�ioctl���õģ�����madplay��û�����ã�����������һ��Ĭ��ֵ(50%)
	wm8978_volume = ((50 & 0xff)*32)/100; 
	wm8978_write_reg(52,((1<<8)|wm8978_volume));
	wm8978_write_reg(53,((1<<8)|wm8978_volume));

//Ĭ��������audio_channelsδ����
	switch (cmd) {
		case SNDCTL_DSP_SETFMT://δ��//////////////////////////NO.3/////////////////////////////////////
			printk("cmd:SNDCTL_DSP_SETFMT,δ��\n");
			get_user(val, (long *) arg);
			/*
			if (val & AUDIO_FMT_MASK) {
				audio_fmt = val;
				break;
			} else
				return -EINVAL;
			*/
		case SNDCTL_DSP_CHANNELS:
			printk("cmd:SNDCTL_DSP_CHANNELS\n");
		case SNDCTL_DSP_STEREO://����˫����//////////////////NO.5//////////////////NO.7////////////////////////////////
			printk("cmd:SNDCTL_DSP_STEREO\n");
			
			get_user(data, (long *) arg);
			audio_channels = data;
			printk("set audio_channels:%d\n",audio_channels);
	    		/* For MONO */
				if (data != 2) {
	        		ar7240_reg_rmw_set(AR7240_STEREO_CONFIG, MONO);      
        		} else {
	        		ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, MONO);      
        		}
				return 0;
/*
			get_user(val, (long *) arg);

			if (cmd == SNDCTL_DSP_STEREO)
				val = val ? 2 : 1;
			if (val != 1 && val != 2)
				return -EINVAL;
			audio_channels = val;
			break;
*/
		case SOUND_PCM_READ_CHANNELS://����������/////////////////NO.4////////////////////NO.6/////////////////////////
			printk("cmd:SOUND_PCM_READ_CHANNELS\n");
			put_user(audio_channels, (long *) arg);
			printk("get audio_channels:%d\n",audio_channels);
			break;

		case SNDCTL_DSP_SPEED://���ò���Ƶ�ʣ�������/////////////////NO.8///////////////////////////////////
			printk("cmd:SNDCTL_DSP_SPEED\n");
			get_user(val, (long *) arg);
			printk("set rate:%d\n",val);
			audio_rate = val;
			//val = audio_set_dsp_speed(val);
			switch (val) {
				case 44100:
					//ar7240_i2s_clk(0x0a47f028, 0x2383);
					//cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_44);
					//cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_44);
					//ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xec330);//gl-inet						   
					ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x11 << 16) + 0xb6b0));//gl-inet 44100kHz
					break;
				case 48000:
					//ar7240_i2s_clk(0x03c9f02c, 0x2383);
					//cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48);
					//cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48);
					//ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xd9013);//gl-inet
					ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x10 << 16) + 0x4600));//gl-inet 48000kHz
					break;
				default:
					printk(KERN_CRIT "Freq %d not supported \n",data);
					return -ENOTSUPP;
			}
			for (j = 0; j < NUM_DESC; j++) {
				dmabuf->db_desc[j].Ca[0] |= cab;
				dmabuf->db_desc[j].Cb[0] |= cab;
				dmabuf->db_desc[j].Ca[1] |= cab1;
				dmabuf->db_desc[j].Cb[1] |= cab1;
			}
			if (val < 0)
				return -EINVAL;
			put_user(val, (long *) arg);
			break;

		case SOUND_PCM_READ_RATE://����Ĭ�ϵĲ�����44100��44800
			printk("cmd:SOUND_PCM_READ_RATE\n");
			put_user(audio_rate, (long *) arg);
			break;

		case SNDCTL_DSP_GETFMTS://����FMT����/////////////////////////NO.2////////////////////////////////////
			printk("cmd:SNDCTL_DSP_GETFMTS,����FMT����\n");
			put_user(AUDIO_FMT_MASK, (long *) arg);
			break;

		case SNDCTL_DSP_GETBLKSIZE://����ÿһfragment��size��С�����ǵ�Ĭ����8k
			printk("cmd:SNDCTL_DSP_GETBLKSIZE\n");
			if(filp->f_mode & FMODE_WRITE)
				//return put_user(audio_fragsize, (long *) arg);
			//else
				//return put_user(audio_fragsize, (int *) arg);
			break;

		case SNDCTL_DSP_SETFRAGMENT://����ÿһfragment��size��С�����ǵ�Ĭ����8k
			printk("cmd:SNDCTL_DSP_SETFRAGMENT\n");
			/*
			if (file->f_mode & FMODE_WRITE) {
				if (output_stream.buffers)//����仰����������ƺ�Ҳ��û��ʵ��
					return -EBUSY;
				get_user(val, (long *) arg);//ͨ������Ӧ�ÿ��Կ�����valֵ�ĺ�16λ����2��fragsize�Ĵ�С��ǰ16Ϊ��fragsize�ĸ���
				audio_fragsize = 1 << (val & 0xFFFF);//֮��������һλ��Ϊ���չ˵����������
				if (audio_fragsize < 16)
					audio_fragsize = 16;
				if (audio_fragsize > 16384)
					audio_fragsize = 16384;
				
				audio_nbfrags = (val >> 16) & 0x7FFF;
				if (audio_nbfrags < 2)
					audio_nbfrags = 2;
				if (audio_nbfrags * audio_fragsize > 128 * 1024)//�������ɼ������ֵΪ128k
					audio_nbfrags = 128 * 1024 / audio_fragsize;
				if (audio_setup_buf(&output_stream))
					return -ENOMEM;

			}
			if (file->f_mode & FMODE_READ) {
				if (input_stream.buffers)
					return -EBUSY;
				get_user(val, (int *) arg);
				audio_fragsize = 1 << (val & 0xFFFF);
				if (audio_fragsize < 16)
					audio_fragsize = 16;
				if (audio_fragsize > 16384)
					audio_fragsize = 16384;
				
				audio_nbfrags = (val >> 16) & 0x7FFF;
				if (audio_nbfrags < 2)
					audio_nbfrags = 2;
				if (audio_nbfrags * audio_fragsize > 128 * 1024)
					audio_nbfrags = 128 * 1024 / audio_fragsize;
				if (audio_setup_buf(&input_stream))
					return -ENOMEM;

			}
			*/
			break;

		case SNDCTL_DSP_SYNC://ͬ��һ����Ƶ����//////////////////NO.1//////////////////////////////////////////////////
			printk("cmd:SNDCTL_DSP_SYNC,audio_sync(file),ͬ��һ����Ƶ����\n");
			//return audio_sync(file);
			break;
		case SNDCTL_DSP_GETOSPACE:
		{
			printk("cmd:SNDCTL_DSP_GETOSPACE\n");
			/*
			audio_stream_t *s = &output_stream;
			audio_buf_info *inf = (audio_buf_info *) arg;
			int err = access_ok(VERIFY_WRITE, inf, sizeof(*inf));
			int i;
			int frags = 0, bytes = 0;

			if (err)
				return err;
			for (i = 0; i < s->nbfrags; i++) {
				if (atomic_read(&s->buffers[i].sem.count) > 0) {
					if (s->buffers[i].size == 0) frags++;
					bytes += s->fragsize - s->buffers[i].size;
				}
			}
			put_user(frags, &inf->fragments);
			put_user(s->nbfrags, &inf->fragstotal);
			put_user(s->fragsize, &inf->fragsize);
			put_user(bytes, &inf->bytes);
			*/
			break;
		}

		case SNDCTL_DSP_GETISPACE://���ؿռ�ʹ�����
		{
			printk("cmd:SNDCTL_DSP_GETISPACE\n");
			/*
			audio_stream_t *s = &input_stream;
			audio_buf_info *inf = (audio_buf_info *) arg;
			//����û��ռ��ڴ���Ƿ���ã���������Ҳ�Ǽ���û��ռ�ָ���Ƿ���á�������ã��򷵻���(��0ֵ)�����򷵻ؼ� (0)
			int err = access_ok(VERIFY_WRITE, inf, sizeof(*inf));
			int i;
			int frags = 0, bytes = 0;

			if (!(file->f_mode & FMODE_READ))
				return -EINVAL;

			if (err)
				return err;
			for(i = 0; i < s->nbfrags; i++){
				if (atomic_read(&s->buffers[i].sem.count) > 0)
				{
					if (s->buffers[i].size == s->fragsize)
						frags++;
					bytes += s->buffers[i].size;
				}
			}
			put_user(frags, &inf->fragments);
			put_user(s->nbfrags, &inf->fragstotal);
			put_user(s->fragsize, &inf->fragsize);
			put_user(bytes, &inf->bytes);
			*/
			break;
		}
		case SNDCTL_DSP_RESET://�����趨
			printk("cmd:SNDCTL_DSP_RESET\n");
				data = arg;
				ar7240_i2sound_dma_resume(data);
				if (data) {
					sc->rpause = 0;
				}else {
					sc->ppause = 0;
				}
        		return 0;
			/*
			if (file->f_mode & FMODE_READ) {
				audio_clear_buf(&input_stream);
			}
			if (file->f_mode & FMODE_WRITE) {
				audio_clear_buf(&output_stream);
			}
			
			return 0;
			*/
		case SNDCTL_DSP_NONBLOCK:
			printk("cmd:SNDCTL_DSP_NONBLOCK\n");
			filp->f_flags |= O_NONBLOCK;
			return 0;
		case SNDCTL_DSP_POST:printk("cmd:SNDCTL_DSP_POST\n");
		case SNDCTL_DSP_SUBDIVIDE:printk("cmd:SNDCTL_DSP_SUBDIVIDE\n");
		case SNDCTL_DSP_GETCAPS:printk("cmd:SNDCTL_DSP_GETCAPS\n");
		case SNDCTL_DSP_GETTRIGGER:printk("cmd:SNDCTL_DSP_GETTRIGGER\n");
		case SNDCTL_DSP_SETTRIGGER:printk("cmd:SNDCTL_DSP_SETTRIGGER\n");
		case SNDCTL_DSP_GETIPTR:printk("cmd:SNDCTL_DSP_GETIPTR\n");
		case SNDCTL_DSP_GETOPTR:printk("cmd:SNDCTL_DSP_GETOPTR\n");
		case SNDCTL_DSP_MAPINBUF:printk("cmd:SNDCTL_DSP_MAPINBUF\n");
		case SNDCTL_DSP_MAPOUTBUF:printk("cmd:SNDCTL_DSP_MAPOUTBUF\n");
		case SNDCTL_DSP_SETSYNCRO:printk("cmd:SNDCTL_DSP_SETSYNCRO\n");
		case SNDCTL_DSP_SETDUPLEX:printk("cmd:SNDCTL_DSP_SETDUPLEX\n");
			return -ENOSYS;
		default:
			return smdk2410_mixer_ioctl(filp, cmd, arg);//����ƺ��Ǻ�Ӳ���й�ϵ��
	}

	return 0;


//end oss
/*

	if (ioctl(audio, I2S_DSIZE, hdr.bit_p_spl) < 0) {
		perror("I2S_DSIZE");
	}

	if (ioctl(audio, I2S_FREQ, hdr.sample_fq) < 0) {
		perror("I2S_FREQ");
	}
*/
		
#if 0
	switch (cmd) {
    		case I2S_PAUSE://��ͣ
        		data = arg;
				ar7240_i2sound_dma_pause(data);
				if (data) {
					sc->rpause = 1;
				} else {
					sc->ppause = 1;
				}
        		return 0;
    		case I2S_RESUME://����
        		data = arg;
				ar7240_i2sound_dma_resume(data);
				if (data) {
					sc->rpause = 0;
				}else {
					sc->ppause = 0;
				}
        		return 0;
			case I2S_VOLUME://����IIS������
				data = arg;
				if (data < 15) {
					//��ֱֵ�Ӷ�ӦdataΪ0x0f0f��������Ӧ���
					if (data < 0) {
						mask = 0xf;
					} else {
						//��֤0~15���ε�ֵ��Ӧ�Ķ���-db��0~15��Ӧ����-84db~+42db
						mask = (~data) & 0xf;
						mask = mask | 0x10;
					}
				} else {
					if (data <= 22) {
						//data=15ʱ���������κβ���������ԭ��������	
						if (data == 15) {
							data = 0;
						} else {
							//��Ӧֱֵ�Ӽ�15����֤����ֵ
							mask = data - 15;
						}
					} else {
						//�����������
						mask = 7;
					}
				}
				data = mask | (mask << 8);
				ar7240_reg_wr(STEREO0_VOLUME, data);
				return 0;

			case I2S_FREQ://���ò���Ƶ��
				data = arg;
        		switch (data) {
            			case 44100:
                			//ar7240_i2s_clk(0x0a47f028, 0x2383);
                			//cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_44);
                			//cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_44);
                			//ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xec330);//gl-inet         			       
    						ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x11 << 16) + 0xb6b0));//gl-inet 44100kHz
                			
               			break;
            			case 48000:
                			//ar7240_i2s_clk(0x03c9f02c, 0x2383);
                			//cab = SPDIF_CONFIG_SAMP_FREQ(SPDIF_SAMP_FREQ_48);
                			//cab1 = SPDIF_CONFIG_ORG_FREQ(SPDIF_ORG_FREQ_48);
                			//ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xd9013);//gl-inet
                			ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x10 << 16) + 0x4600));//gl-inet 48000kHz
                		break;
           			default:
                		printk(KERN_CRIT "Freq %d not supported \n",data);
                		return -ENOTSUPP;
        		}
        		for (j = 0; j < NUM_DESC; j++) {
            			dmabuf->db_desc[j].Ca[0] |= cab;
            			dmabuf->db_desc[j].Cb[0] |= cab;
            			dmabuf->db_desc[j].Ca[1] |= cab1;
            			dmabuf->db_desc[j].Cb[1] |= cab1;
        		}
				return 0;

			case I2S_FINE:
				data = arg;
				return 0;

			case I2S_DSIZE://���ò���λ��
				data = arg;
				switch (data) {
					case 8:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
						  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_8B)
						  		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16);
           				break;
				
					case 16:
            			st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
            			  		AR7240_STEREO_CONFIG_PCM_SWAP |
                 		  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B)
                 		  		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_8_16);
            			break;
				
					case 24:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
                		  		AR7240_STEREO_CONFIG_PCM_SWAP |
                		  		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_24B) |
                 		  		AR7240_STEREO_CONFIG_I2S_32B_WORD
                 		  		);
           				cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32);
            			break;
					case 32:
						st_cfg = (AR7240_STEREO_CONFIG_DEFAULT | 
                		  		AR7240_STEREO_CONFIG_PCM_SWAP |
                	      		AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_32B) |
                 	      		AR7240_STEREO_CONFIG_I2S_32B_WORD
                 	      		);
            			cab1 = SPDIF_CONFIG_SAMP_SIZE(SPDIF_S_24_32);
            			break;
					default:
						printk(KERN_CRIT "Data size %d not supported \n",data);
						return -ENOTSUPP;
				}
        		ar7240_reg_wr(AR7240_STEREO_CONFIG, (st_cfg | AR7240_STEREO_CONFIG_RESET));
        		udelay(100);
				ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG,AR7240_STEREO_CONFIG_RESET);//�������λλ����Ȼʱ��ֹͣ���
        		ar7240_reg_wr(AR7240_STEREO_CONFIG, st_cfg);
        		for (j = 0; j < NUM_DESC; j++) {
            		dmabuf->db_desc[j].Ca[1] |= cab1;
            		dmabuf->db_desc[j].Cb[1] |= cab1;
        		}
        		return 0;

			case I2S_MODE://��������˫����
				data = arg;
	    		/* For MONO */
				if (data != 2) {
	        		ar7240_reg_rmw_set(AR7240_STEREO_CONFIG, MONO);      
        		} else {
	        		ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, MONO);      
        		}
				return 0;

        	case I2S_MCLK:    //������Ƶ   /* Master clock is MCLK_IN or divided audio PLL */
				data = arg;
        		if (data) {
            		ar7240_reg_wr(AUDIO_PLL, AUDIO_PLL_RESET); /* Reset Audio PLL */
            		ar7240_reg_rmw_set(AR7240_STEREO_CONFIG, AR7240_STEREO_CONFIG_I2S_MCLK_SEL);
        		} else {
            		ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, AR7240_STEREO_CONFIG_I2S_MCLK_SEL);
				}
				return 0;

        	case I2S_COUNT:
				data = arg;
				return 0;

			default:
				return -ETOOSMALL;
		}
#endif


}

irqreturn_t ar7240_i2s_intr(int irq, void *dev_id)
//irq_handler_t ar7240_i2s_intr(int irq, void *dev_id, struct pt_regs *regs)
{
	uint32_t r;
	//MBOX Rx DMA completion (one descriptor completed) interrupts
	//��ȡMBOX�жϼĴ���
	r = ar7240_reg_rd(MBOX_INT_STATUS);
   
    if(r & RX_UNDERFLOW)//MBOX Rx���������������������ӿ�����Ĵ�����ͬ
        stats.rx_underflow++;

	/* Ack the interrupts */
	ar7240_reg_wr(MBOX_INT_STATUS, r);//Ϊʲô�ְѶ�ȡ����Ϣд��ȥ?????????

	return IRQ_HANDLED;
}
//��ʼ��IIS�Ĵ�����ʹ��ͨ�ܽ����IIS�ź�
void ar7240_i2sound_i2slink_on(int master)
{
    /* Clear all resets */   
    ar7240_reg_wr(RST_RESET, 0);
    udelay(500);

	/*Set GPIO control wm8978 */

	//ar7240_reg_wr(AR7240_GPIO_OE,(AR7240_LED_3|AR7240_LED_4|AR7240_LED_5));	 
	ar7240_reg_wr(AR7240_GPIO_OE,(IIS_CONTROL_CSB|IIS_CONTROL_SDIN|IIS_CONTROL_SCLK));	

    /* Set GPIO I2S Enables */
    /*
     * 0x3c000001 should be set to GPIO for MIC and I2S
     * 0x7c000001 should be set to GPIO for SPDIF
      *����ΪI2Sģʽ     
      *AR7240_GPIO_FUNCTION_SPDIF_EN:Enables GPIO_23 or TCK as the SPDIF serial output     
      *AR7240_GPIO_FUNCTION_I2S_MCKEN:Enables the master audio CLK_MCK to be output through GPIO_21. \     
      *Works only if I2S0_22_18_EN (bit [29]) is also set     
      *AR7240_GPIO_FUNCTION_I2S0_EN:Enables I2S functions on GPIO pins
     */
    ar7240_reg_rmw_set(AR7240_GPIO_FUNCTIONS,
    	(AR7240_GPIO_FUNCTION_SPDIF_EN |
        AR7240_GPIO_FUNCTION_I2S_MCKEN | 
		AR7240_GPIO_FUNCTION_I2S0_EN|
		AR7240_GPIO_FUNCTION_I2S_GPIO_18_22_EN
		));
#if 0
    /*
     * GPIO 13 and GPIO 14 are used for I2S logic.
     * So disabling LED on these pins.
     */
     //���GPIO_13��GPIO_14λ����������:Enables the Ethernet Switch LED data on GPIO_13��GPIO_14
    ar7240_reg_rmw_clear(AR7240_GPIO_FUNCTIONS,
    	(AR7240_GPIO_FUNCTION_ETH_SWITCH_LED1_EN |
        AR7240_GPIO_FUNCTION_ETH_SWITCH_LED0_EN
        ));
//AR7240_GPIO_FUNCTION_2_EN_I2WS_ON_0 :Enables I2S_WS on GPIO_0	
//AR7240_GPIO_FUNCTION_2_EN_I2SCK_ON_1:Enables I2S_CK Out on GPIO_1	
//AR7240_GPIO_FUNCTION_2_I2S_ON_LED   :Brings out I2S related signals on pins GPIO_14, GPIO_15 and GPIO_16
    ar7240_reg_rmw_set(AR7240_GPIO_FUNCTION_2,
        (AR7240_GPIO_FUNCTION_2_EN_I2WS_ON_0 |
        AR7240_GPIO_FUNCTION_2_EN_I2SCK_ON_1 |
        AR7240_GPIO_FUNCTION_2_I2S_ON_LED));
#endif
    /* AR7240_STEREO_CONFIG should carry 0x201302 for MIC and I2S     
     * AR7240_STEREO_CONFIG should carry 0xa01302 for SPDIF     
     */     
     //AR7240_STEREO_CONFIG_PCM_SWAP:This bit is used for swapping byte order of PCM samples     
     //AR7240_STEREO_CONFIG_SPDIF_ENABLE:Enables the SPDIF stereo block for operation     
     //AR7240_STEREO_CONFIG_RESET:Resets the stereo buffers and I2S state; Should be written to 1 when any of      							
     //the data word sizes change, or if data synchronization is lost. Hardware will                                
     //automatically clear to 0     //AR7240_STEREO_CONFIG_ENABLE:Enables operation of the I2S stereo block     
     //AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B):             
     //Controls the word size loaded into the PCM register from the MBOX FIFO.             
     //Data word size:             
     		//0x0 8 bits/word             
     		//0x1 16 bits/word             
     		//0x2 24 bits/word             
     		//0x3 32 bits/word     
     		//���ó�16λ����32λ��ֱ��Ӱ�쵽WSƵ�ʺ�BITCLK�Ĺ�ϵ�������32λ����WS=BITCLK/32*2     
     		//�����16λ����WS=BITCLK/16*2�������Ǹ�������ѡ���32λ��������ѡ��16��������о͸�һ��     
     		//AR7240_STEREO_CONFIG_SAMPLE_CNT_CLEAR_TYPE:see datasheet     
     		//AR7240_STEREO_CONFIG_MASTER:The AR9331 is the master and drives I2S_CK and I2S_WS     
     		//AR7240_STEREO_CONFIG_PSEDGE(2):see datasheet     //
    ar7240_reg_wr(AR7240_STEREO_CONFIG,
        (AR7240_STEREO_CONFIG_SPDIF_ENABLE|
        AR7240_STEREO_CONFIG_ENABLE|
        AR7240_STEREO_CONFIG_RESET|
        AR7240_STEREO_CONFIG_PCM_SWAP|
        AR7240_STEREO_CONFIG_MIC_WORD_SIZE|
		AR7240_STEREO_CONFIG_MODE(0)|
        AR7240_STEREO_CONFIG_DATA_WORD_SIZE(AR7240_STEREO_WS_16B)|
		AR7240_STEREO_CONFIG_I2S_32B_WORD|
		AR7240_STEREO_CONFIG_SAMPLE_CNT_CLEAR_TYPE|
        AR7240_STEREO_CONFIG_MASTER|
		AR7240_STEREO_CONFIG_PSEDGE(2)
        ));//gl-inet
  
    //ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xec330);//gl-inet
    //ar7240_reg_wr(AR7240_STEREO_CLK_DIV,0xd9013);//gl-inet
    //ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x10 << 16) + 0x4600));//gl-inet 48000kHz   
    ar7240_reg_wr(AR7240_STEREO_CLK_DIV,((0x11 << 16) + 0xb6b0));//gl-inet 44100kHz

	init_wm8978();
	

#if 0	
	{			
		ar7240_gpio_setpin(AR7240_LED_8,1);				
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_4,1);				
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_5,1);				
		mdelay(200);			
		//mdelay(500);					
		ar7240_gpio_setpin(AR7240_LED_8,0);					
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_4,0);			
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_5,0);			
		mdelay(200);

		ar7240_gpio_setpin(AR7240_LED_8,1);				
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_4,1);				
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_5,1);				
		mdelay(200);
				
		ar7240_gpio_setpin(AR7240_LED_8,0);					
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_4,0);			
		mdelay(200);			
		ar7240_gpio_setpin(AR7240_LED_5,0);			
		mdelay(200);
			//mdelay(500);						
	}
#endif
    udelay(100);
    ar7240_reg_rmw_clear(AR7240_STEREO_CONFIG, AR7240_STEREO_CONFIG_RESET);
}

void ar7240_i2sound_request_dma_channel(int mode)
{
	ar7240_reg_wr(MBOX_DMA_POLICY, 0x6a);
}

void ar7240_i2sound_dma_desc(unsigned long desc_buf_p, int mode)
{
	/*
	 * Program the device to generate interrupts
	 * RX_DMA_COMPLETE for mbox 0
	 */
	if (mode) {
		ar7240_reg_wr(MBOX0_DMA_TX_DESCRIPTOR_BASE, desc_buf_p);
	} else {
		ar7240_reg_wr(MBOX0_DMA_RX_DESCRIPTOR_BASE, desc_buf_p);
	}
}

void ar7240_i2sound_dma_start(int mode)
{
	/*
	 * Start
	 */
	if (mode) {
		ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, START);
	} else {
		ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, START);
	}
}


void ar7240_i2sound_dma_pause(int mode)
{
        /*
         * Pause
         */
        if (mode) {
                ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, PAUSE);
        } else {
                ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, PAUSE);
        }
}

void ar7240_i2sound_dma_resume(int mode)
{
        /*MBOX_STATUS
         * Resume
         */
        if (mode) {
                ar7240_reg_wr(MBOX0_DMA_TX_CONTROL, RESUME);
        } else {
                ar7240_reg_wr(MBOX0_DMA_RX_CONTROL, RESUME);
        }
}

//Ϊʵ�ֺ���
loff_t ar7240_i2s_llseek(struct file *filp, loff_t off, int whence)
{
	printk(KERN_CRIT "llseek\n");
	return off;
}


struct file_operations ar7240_i2s_fops = {
	.owner = THIS_MODULE,
	.llseek = ar7240_i2s_llseek,
	.write = ar9100_i2s_write,
	.unlocked_ioctl = ar7240_i2s_ioctl,
	.open = ar7240_i2s_open,
	.release = ar7240_i2s_close,
	.read = ar7240_i2s_read,
};

//���������
int ar7240_i2s_init_module(void)
{

	ar7240_i2s_softc_t *sc = &sc_buf_var;
	int result = -1;
	int master = 1;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	 
#ifdef GLZT_I2S_310KERNEL_CLASS
/*
	dev_t devno = MKDEV(ar7240_i2s_major, ar7240_i2s_minor);

	result = register_chrdev_region(devno,ar7240_i2s_num, "glzt_i2s");
	if (result < 0) {
		printk("register_chrdev_region\n");
		return result;
	}

	cdev_init(&cdev, &ar7240_i2s_fops);
	cdev.owner = THIS_MODULE;
	
	result  = cdev_add(&cdev, devno, ar7240_i2s_num);
	if (result  < 0) {
		printk("cdev_add\n");
		goto err1;
	}

	glzt_i2s_class = class_create(THIS_MODULE, "glzt_i2s");
	glzt_i2s_class_dev = device_create(glzt_i2s_class, NULL, devno, NULL, "i2s"); 
*/
	audio_dev_dsp = register_sound_dsp(&ar7240_i2s_fops, -1);
	audio_dev_mixer = register_sound_mixer(&smdk2410_mixer_fops, -1);

#else
	dev_t devno = MKDEV(ar7240_i2s_major, ar7240_i2s_minor);

	

	if (ar7240_i2s_major) {
			/* Establish ISR would take care of enabling the interrupt */
		result = register_chrdev(ar7240_i2s_major, "ath_i2s",
				    &ar7240_i2s_fops);//ע�������豸����3.10��ģʽ��̫һ�����������ã�markһ��
	}
	if (result < 0) {
		printk(KERN_WARNING "ar7240_i2s: can't get major %d\n",
		       ar7240_i2s_major);
		return result;
	}
	glzt_i2s_class = class_create(THIS_MODULE, "glzt_i2s");
	//glzt_i2s_class_dev = device_create(glzt_i2s_class, NULL, devno, NULL, "i2s"); /* /dev/buttons */
	glzt_i2s_class_dev = device_create(glzt_i2s_class, NULL, devno, NULL, "dsp");
	
#endif


	sc->sc_irq = AR7240_MISC_IRQ_DMA;

	/* Establish ISR would take care of enabling the interrupt */
	//ע��DMA�жϣ�misc���жϺ���0x10+7=23������ȷ���Ƿ���ȷ�������ֲῴ�����жϺ�
	result = request_irq(sc->sc_irq, ar7240_i2s_intr, IRQF_DISABLED,"ar7240_i2s", NULL);
	if (result) {
		printk(KERN_INFO
		       "i2s: can't get assigned irq %d returns %d\n",sc->sc_irq, result);
		
	}
	//��ʼ��IIS�Ĵ�����ʹ��ͨ�ܽ����IIS�ź�
	ar7240_i2sound_i2slink_on(master);

	I2S_LOCK_INIT(&sc_buf_var);//��ʼ��������

	return 0;		/* succeed */

//err2:
	//cdev_del(&cdev);

//err1:
	//unregister_chrdev_region(devno, ar7240_i2s_num);
//	return result;

}
//�����ĳ���
void ar7240_i2s_cleanup_module(void)
{
	ar7240_i2s_softc_t *sc = &sc_buf_var;
	
#ifdef GLZT_I2S_310KERNEL_CLASS
/*
	dev_t devno = MKDEV(ar7240_i2s_major, ar7240_i2s_minor);
	free_irq(sc->sc_irq, NULL);
	device_unregister(glzt_i2s_class_dev);
	class_destroy(glzt_i2s_class);
	cdev_del(&cdev);
	unregister_chrdev_region(devno, ar7240_i2s_num);
	printk("glzt_i2s exit\n");
*/
	free_irq(sc->sc_irq, NULL);
	unregister_sound_dsp(audio_dev_dsp);
	unregister_sound_mixer(audio_dev_mixer);

#else

	printk(KERN_CRIT "unregister\n");
	free_irq(sc->sc_irq, NULL);//ע���ж�
	device_unregister(glzt_i2s_class_dev);
	class_destroy(glzt_i2s_class);
	unregister_chrdev(ar7240_i2s_major, "ath_i2s");//ɾ���豸
	
#endif

}



module_init(ar7240_i2s_init_module);
module_exit(ar7240_i2s_cleanup_module);
