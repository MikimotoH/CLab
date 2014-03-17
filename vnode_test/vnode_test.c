#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/fcntl.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#include "pi_util.h"

#define BUFFER_SIZE	256

MALLOC_DECLARE(M_VNODE_TEST);
MALLOC_DEFINE(M_VNODE_TEST, "vnode_test buffer", "vnode_test buffer");

#define K_MALLOC(p, n) malloc(p, n, M_VNODE_TEST, M_NOWAIT| M_ZERO )
#define K_REALLOC(p, n) reallocf(p, n, M_VNODE_TEST, M_NOWAIT| M_ZERO )
#define K_FREE(p) free(p, M_VNODE_TEST )

#define LOGTRC(fmtstr, ...)  \
    uprintf("<TRACE>%s:%u: " fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)
#define LOGCRI(fmtstr, ...)  \
    uprintf("<CRIT>%s:%u: " fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define __cleanup(func)  __attribute__((cleanup(func)))

static struct cdev *vnode_test_dev;

typedef int32_t i32;
typedef uint32_t u32;


static int
vnode_test_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
    LOGTRC("open");
    return (0);
}

static int
vnode_test_close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
    LOGTRC("close");
    return (0);
}
#define bad_end(ret, msg,...) ({LOGCRI(msg,##__VA_ARGS__); ret;})

static int 
vnode_open(const char* filename, int* flags, OUT struct vnode ** vn)
{
    struct nameidata nd;
    int vfslocked;
    int error=0;

    NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | MPSAFE, UIO_SYSSPACE, filename, curthread);
    error = vn_open(&nd, flags, 0, NULL);
    if (error)
        return bad_end(error, "vn_open(\"%s\") failed, error=%d", filename, error);

    vfslocked = NDHASGIANT(&nd);
    *vn = nd.ni_vp;

    VOP_UNLOCK(*vn, 0);
    NDFREE(&nd, NDF_ONLY_PNBUF);
    VFS_UNLOCK_GIANT(vfslocked);
    return 0;
}

static int 
vnode_write(struct vnode* vn, const void* data, size_t data_len)
{

    // Start writing
    int vfslocked = VFS_LOCK_GIANT(vn->v_mount);
    struct mount *mountpoint = NULL;
    vn_start_write(vn, &mountpoint, V_WAIT);
    vn_lock(vn, LK_EXCLUSIVE | LK_RETRY);

    struct iovec iov={0};
    iov.iov_base=(void*)data;
    iov.iov_len =data_len;

    struct uio uio={0};
    bzero(&uio, sizeof(uio));
    uio.uio_rw = UIO_WRITE;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_offset = 0;
    uio.uio_resid = iov.iov_len;
    uio.uio_iov = &iov;
    uio.uio_iovcnt = 1;
    uio.uio_td = curthread;

    // Show debug info: uio and iovec contents
    int error = VOP_WRITE(vn, &uio, IO_UNIT, curthread->td_ucred);

    if (error)
        LOGCRI("VOP_WRITE() failed (error=%d)", error);
    VOP_UNLOCK(vn, 0);
    vn_finished_write(mountpoint);
    VFS_UNLOCK_GIANT(vfslocked);
    return 0;
}

static void
vnode_close(int flags, struct vnode *vn)
{
    vn_close(vn, flags, 0, curthread);
}

static const char* g_filename="/root/vnode_test.txt";
static char g_data[256];

static int
vnode_test_write(struct cdev *dev, struct uio *uio, int ioflag)
{
    LOGTRC("uio->uio_iov->iov_len=%lu, BUFFER_SIZE-1=%u", 
            uio->uio_iov->iov_len, BUFFER_SIZE-1);
    if(uio->uio_iov->iov_len >= sizeof(g_data))
        return bad_end(E2BIG, 
                "uio->uio_iov->iov_len(%lu) is bigger than sizeof(g_data)=%lu"
                , uio->uio_iov->iov_len, sizeof(g_data));

    int error = copyin(uio->uio_iov->iov_base, g_data, uio->uio_iov->iov_len );
    LOGTRC("copyin() returned error=%d, len=%lu", error, uio->uio_iov->iov_len);
    if (error) 
        return bad_end(error,"copyin() failed, return %d", error);

    g_data[uio->uio_iov->iov_len-1] = 0;
    LOGTRC("g_data=\"%s\"", g_data);
    struct vnode *vnode= NULL;
    int flags = FWRITE|O_TRUNC|O_CREAT;
    error = vnode_open(g_filename, &flags, &vnode);
    if(error)
        return bad_end(error,"vnode_open() failed, error=%d", error);
    error = vnode_write(vnode, g_data, strnlen(g_data, sizeof(g_data)));
    if(error)
        LOGCRI("vnode_write(\"%s\") failed", g_filename);
    vnode_close(flags, vnode);

    return (error);
}

static struct cdevsw vnode_test_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	vnode_test_open,
	.d_close =	vnode_test_close,
	.d_write =	vnode_test_write,
	.d_name =	"vnode_test"
};

static int
vnode_test_modevent(module_t mod __unused, int event, void *arg __unused)
{
    int error = 0;

    switch (event) {
        case MOD_LOAD:
            vnode_test_dev = make_dev(&vnode_test_cdevsw, 0, UID_ROOT, GID_WHEEL,
                    0600, "vnode_test");
            uprintf("vnode_test driver loaded.\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(vnode_test_dev);
            uprintf("vnode_test driver unloaded.\n");
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return (error);
}

DEV_MODULE(vnode_test, vnode_test_modevent, NULL);

