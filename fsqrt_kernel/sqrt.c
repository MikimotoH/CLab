#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#define BUFFER_SIZE	256

MALLOC_DECLARE(M_SQRT);
MALLOC_DEFINE(M_SQRT, "sqrt_buffer", "buffer for sqrt driver");

#define LOGTRC(fmtstr, ...)  \
    uprintf("<TRACE>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

static d_open_t		sqrt_open;
static d_close_t	sqrt_close;
static d_read_t		sqrt_read;
static d_write_t	sqrt_write;

static struct cdevsw sqrt_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	sqrt_open,
	.d_close =	sqrt_close,
	.d_read =	sqrt_read,
	.d_write =	sqrt_write,
	.d_name =	"sqrt"
};

typedef struct sqrt {
	char buffer[BUFFER_SIZE];
	int length;
} sqrt_t;

static sqrt_t *sqrt_message;
static struct cdev *sqrt_dev;



static inline long double 
asmSqrt(long double x) 
{
    long double o = 0;
_Static_assert(sizeof(long double)==16,"");
    __asm__ __volatile__ (
            "fldt %1;"
            "fsqrt;"
            "fstpt %0;"
            : "=g" (o) /* output, %0 */
            : "g"  (x) /* input,  %1, %2, %3, ... */
             /* : list of clobbered registers      optional */
            );
    return o;
}

typedef int32_t i32;
typedef uint32_t u32;

static int
sqrt_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    LOGTRC("open");
    return (0);
}

static int
sqrt_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    LOGTRC("close");
    return (0);
}

static int
sqrt_write(struct cdev *dev, struct uio *uio, int ioflag)
{
    LOGTRC("uio->uio_iov->iov_len=%lu, BUFFER_SIZE-1=%u", 
            uio->uio_iov->iov_len, BUFFER_SIZE-1);

    size_t bytescopied = MIN(uio->uio_iov->iov_len, BUFFER_SIZE-1);

    int error = copyin(uio->uio_iov->iov_base, sqrt_message->buffer, 
            bytescopied );
    LOGTRC("copyin() returned error=%d, bytescopied=%lu", error, bytescopied);
    if (error != 0) {
        uprintf("Write failed.\n");
        return (error);
    }
    sqrt_message->buffer[bytescopied-1] = 0;

    LOGTRC("sqrt_message->buffer=\"%s\"", sqrt_message->buffer);
    u32 number = (u32)strtoul(sqrt_message->buffer, NULL, 10 );
    LOGTRC("strtol() returned %u", number);

    double s = asmSqrt(number);
    double fraction = s - (double)(u32)s;
    LOGTRC("asmSqrt(%u) == %u.%05u", number, 
           (u32)(s), (u32)(fraction*100000));

    sqrt_message->length = bytescopied; 
    LOGTRC("sqrt_message->length=%u", sqrt_message->length);

    return (error);
}

static int
sqrt_read(struct cdev *dev, struct uio *uio, int ioflag)
{
    int error = 0;
    int amount;

    LOGTRC("uio->uio_resid=%lu, sqrt_message->length=%d, uio->uio_offset=%ld",
            uio->uio_resid, sqrt_message->length, uio->uio_offset);
    amount = MIN(uio->uio_resid,
            (sqrt_message->length - uio->uio_offset > 0) ?
            sqrt_message->length - uio->uio_offset : 0);
    LOGTRC("amout=%d", amount);

    error = uiomove(sqrt_message->buffer + uio->uio_offset, amount, uio);
    LOGTRC("uiomove() returned error=%d", error);
    if (error != 0)
        uprintf("Read failed.\n");

    return (error);
}

static int
sqrt_modevent(module_t mod __unused, int event, void *arg __unused)
{
    int error = 0;

    switch (event) {
        case MOD_LOAD:
            sqrt_message = malloc(sizeof(sqrt_t), M_SQRT, M_WAITOK);
            sqrt_dev = make_dev(&sqrt_cdevsw, 0, UID_ROOT, GID_WHEEL,
                    0600, "sqrt");
            uprintf("sqrt driver loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(sqrt_dev);
            free(sqrt_message, M_SQRT);
            uprintf("sqrt driver unloaded.\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return (error);
}

DEV_MODULE(sqrt, sqrt_modevent, NULL);

