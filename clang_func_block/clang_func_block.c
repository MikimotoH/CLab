#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <Block.h>

#include "pi_util.h"

#define BUFFER_SIZE	256

MALLOC_DECLARE(M_CLANG_FUNC_BLOCK);
MALLOC_DEFINE(M_CLANG_FUNC_BLOCK, "clang_func_block buffer", "clang_func_block buffer");

#define KREALLOC(p, n) reallocf(p, n, M_CLANG_FUNC_BLOCK, M_WAITOK | M_ZERO )
#define KFREE(p) free(p, M_CLANG_FUNC_BLOCK )

#define LOGTRC(fmtstr, ...)  \
    uprintf("<TRACE>%s:%u: " fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)
#define LOGCRI(fmtstr, ...)  \
    uprintf("<CRIT>%s:%u: " fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define __cleanup(func)  __attribute__((cleanup(func)))


typedef struct{
    char str[256];
}short_text_t;

#define T  short_text_t
#include "vector_t.h"
#undef T



static d_open_t		clang_func_block_open;
static d_close_t	clang_func_block_close;
static d_read_t		clang_func_block_read;
static d_write_t	clang_func_block_write;

static struct cdevsw clang_func_block_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	clang_func_block_open,
	.d_close =	clang_func_block_close,
	.d_read =	clang_func_block_read,
	.d_write =	clang_func_block_write,
	.d_name =	"clang_func_block"
};

typedef struct clang_func_block {
	char buffer[BUFFER_SIZE];
	int length;
} clang_func_block_t;

static clang_func_block_t *clang_func_block_message;
static struct cdev *clang_func_block_dev;


typedef int32_t i32;
typedef uint32_t u32;

void * _NSConcreteStackBlock[32] = { 0 };

static void 
walk_every_place( void (^visitor)(int y) )
{
    visitor(-3);
}

static inline __always_inline short_text_t 
make_short_text(const char* str)
{
    short_text_t r;
    strlcpy(r.str, str, sizeof(r.str));
    return r;
}

static void test_vector(void)
{
    LOGTRC("enter");
    __cleanup(vector_clear) vector_t v ={ 0 };
    vector_append(&v, make_short_text("Apple"));
    vector_append(&v, make_short_text("Banana"));
    vector_append(&v, make_short_text("Cherry"));
    vector_append(&v, make_short_text("Durian"));
    vector_append(&v, make_short_text("Elephant"));
    vector_append(&v, make_short_text("Firefox"));
    vector_append(&v, make_short_text("Ghostwriter"));
    vector_append(&v, make_short_text("Homeland Security"));
    vector_append(&v, make_short_text("Illinois"));
    vector_append(&v, make_short_text("Jeremy"));
    vector_append(&v, make_short_text("Klukluklux"));
    vector_append(&v, make_short_text("Led Zeppline"));
    vector_append(&v, make_short_text("Mammoth"));
    vector_append(&v, make_short_text("Negromonia"));
    vector_append(&v, make_short_text("Ophelia"));
    vector_append(&v, make_short_text("Puritans"));
    vector_append(&v, make_short_text("Que sera sera"));
    vector_append(&v, make_short_text("Rolling stone"));
    vector_append(&v, make_short_text("Street Figther"));
    vector_append(&v, make_short_text("Thorton"));
    vector_append(&v, make_short_text("Ulysses"));
    vector_append(&v, make_short_text("Veni Vidi Vici"));
    vector_append(&v, make_short_text("Whopper"));
    vector_append(&v, make_short_text("X-men"));
    vector_append(&v, make_short_text("ZZtop"));
}

static int
clang_func_block_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    LOGTRC("open");
    int x = 10;
    void (^vv)(int y) = ^(int y){ LOGTRC("x is %d, y=%d", x, y); };
    x = 11;
    vv(-3);
    walk_every_place(vv);

    test_vector();
    return (0);
}

static int
clang_func_block_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    LOGTRC("close");
    return (0);
}

static int
clang_func_block_write(struct cdev *dev, struct uio *uio, int ioflag)
{
    LOGTRC("uio->uio_iov->iov_len=%lu, BUFFER_SIZE-1=%u", 
            uio->uio_iov->iov_len, BUFFER_SIZE-1);

    size_t bytescopied = MIN(uio->uio_iov->iov_len, BUFFER_SIZE-1);

    int error = copyin(uio->uio_iov->iov_base, clang_func_block_message->buffer, 
            bytescopied );
    LOGTRC("copyin() returned error=%d, bytescopied=%lu", error, bytescopied);
    if (error != 0) {
        uprintf("Write failed.\n");
        return (error);
    }
    clang_func_block_message->buffer[bytescopied-1] = 0;

    LOGTRC("clang_func_block_message->buffer=\"%s\"", clang_func_block_message->buffer);
    u32 number = (u32)strtoul(clang_func_block_message->buffer, NULL, 10 );
    LOGTRC("strtol() returned %u", number);


    clang_func_block_message->length = bytescopied; 
    LOGTRC("clang_func_block_message->length=%u", clang_func_block_message->length);

    return (error);
}

static int
clang_func_block_read(struct cdev *dev, struct uio *uio, int ioflag)
{
    int error = 0;
    int amount;

    LOGTRC("uio->uio_resid=%lu, clang_func_block_message->length=%d, uio->uio_offset=%ld",
            uio->uio_resid, clang_func_block_message->length, uio->uio_offset);
    amount = MIN(uio->uio_resid,
            (clang_func_block_message->length - uio->uio_offset > 0) ?
            clang_func_block_message->length - uio->uio_offset : 0);
    LOGTRC("amout=%d", amount);

    error = uiomove(clang_func_block_message->buffer + uio->uio_offset, amount, uio);
    LOGTRC("uiomove() returned error=%d", error);
    if (error != 0)
        uprintf("Read failed.\n");

    return (error);
}

static int
clang_func_block_modevent(module_t mod __unused, int event, void *arg __unused)
{
    int error = 0;

    switch (event) {
        case MOD_LOAD:
            clang_func_block_message = malloc(sizeof(clang_func_block_t), M_CLANG_FUNC_BLOCK, M_WAITOK);
            clang_func_block_dev = make_dev(&clang_func_block_cdevsw, 0, UID_ROOT, GID_WHEEL,
                    0600, "clang_func_block");
            uprintf("clang_func_block driver loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(clang_func_block_dev);
            free(clang_func_block_message, M_CLANG_FUNC_BLOCK);
            uprintf("clang_func_block driver unloaded.\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return (error);
}

DEV_MODULE(clang_func_block, clang_func_block_modevent, NULL);

