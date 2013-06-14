/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"
#include "qeglfscursor.h"

#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include <private/qmath_p.h>
#include <private/qcore_unix_p.h>

#include <hwcomposer_window.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <malloc.h>
#include <sync/sync.h>

QT_BEGIN_NAMESPACE

// file descriptor for the frame buffer
// this is a global static to keep the QEglFSHooks interface as clean as possible
static int framebuffer = -1;
static hw_module_t *hwcModule = 0;
static hwc_composer_device_1_t *hwcDevicePtr = 0;
static HWComposerNativeWindow *hwc_win = 0;
static hwc_display_contents_1_t *hwc_list = 0;
static hwc_display_contents_1_t **hwc_mList = 0;
static int oldretire = -1, oldrelease = -1;


const char *QEglFSHooks::fbDeviceName() const
{
    return "/dev/fb0";
}

void QEglFSHooks::platformInit()
{
    int err;
    framebuffer = qt_safe_open(fbDeviceName(), O_RDONLY);

    if (framebuffer == -1)
        qWarning("EGLFS: Failed to open %s", fbDeviceName());

    err = hw_get_module(HWC_HARDWARE_MODULE_ID, (const hw_module_t **) &hwcModule);
    assert(err == 0);

    err = hwc_open_1(hwcModule, &hwcDevicePtr);
    assert(err == 0);

    hwcDevicePtr->blank(hwcDevicePtr, 0, 0);
}

void QEglFSHooks::platformDestroy()
{
    if (framebuffer != -1)
        close(framebuffer);
}

EGLNativeDisplayType QEglFSHooks::platformDisplay() const
{
    return EGL_DEFAULT_DISPLAY;
}

QSizeF QEglFSHooks::physicalScreenSize() const
{
    static QSizeF size;
    if (size.isEmpty()) {

        // Note: in millimeters
        int width = qgetenv("QT_QPA_EGLFS_PHYSICAL_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_PHYSICAL_HEIGHT").toInt();

        if (width && height) {
            // no need to read fb0
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;
        int w = -1;
        int h = -1;
        QSize screenResolution;

        if (framebuffer != -1) {
            if (ioctl(framebuffer, FBIOGET_VSCREENINFO, &vinfo) == -1) {
                qWarning("EGLFS: Could not query variable screen info.");
            } else {
                w = vinfo.width;
                h = vinfo.height;
                screenResolution = QSize(vinfo.xres, vinfo.yres);
            }
        } else {
            screenResolution = screenSize();
        }

        const int defaultPhysicalDpi = 100;
        size.setWidth(w <= 0 ? screenResolution.width() * Q_MM_PER_INCH / defaultPhysicalDpi : qreal(w));
        size.setHeight(h <= 0 ? screenResolution.height() * Q_MM_PER_INCH / defaultPhysicalDpi : qreal(h));

        if (w <= 0 || h <= 0) {
            qWarning("EGLFS: Unable to query physical screen size, defaulting to %d dpi.\n"
                     "EGLFS: To override, set QT_QPA_EGLFS_PHYSICAL_WIDTH "
                     "and QT_QPA_EGLFS_PHYSICAL_HEIGHT (in millimeters).",
                     defaultPhysicalDpi);
        }

        // override fb0 from environment var setting
        if (width)
            size.setWidth(width);
        if (height)
            size.setWidth(height);
    }
    return size;
}

QSize QEglFSHooks::screenSize() const
{
    static QSize size;

    if (size.isEmpty()) {
        int width = qgetenv("QT_QPA_EGLFS_WIDTH").toInt();
        int height = qgetenv("QT_QPA_EGLFS_HEIGHT").toInt();

        if (width && height) {
            // no need to read fb0
            size.setWidth(width);
            size.setHeight(height);
            return size;
        }

        struct fb_var_screeninfo vinfo;

        int xres = -1;
        int yres = -1;

        if (framebuffer != -1) {
            if (ioctl(framebuffer, FBIOGET_VSCREENINFO, &vinfo) == -1) {
                qWarning("EGLFS: Could not query variable screen info.");
            } else {
                xres = vinfo.xres;
                yres = vinfo.yres;
            }
        }

        const int defaultWidth = 800;
        const int defaultHeight = 600;
        size.setWidth(xres <= 0 ? defaultWidth : xres);
        size.setHeight(yres <= 0 ? defaultHeight : yres);

        if (xres <= 0 || yres <= 0) {
            qWarning("EGLFS: Unable to query screen resolution, defaulting to %dx%d.\n"
                     "EGLFS: To override, set QT_QPA_EGLFS_WIDTH and QT_QPA_EGLFS_HEIGHT.",
                     defaultWidth, defaultHeight);
        }

        // override fb0 from environment var setting
        if (width)
            size.setWidth(width);
        if (height)
            size.setHeight(height);
    }

    return size;
}

QDpi QEglFSHooks::logicalDpi() const
{
    QSizeF ps = physicalScreenSize();
    QSize s = screenSize();

    return QDpi(25.4 * s.width() / ps.width(),
                25.4 * s.height() / ps.height());
}

int QEglFSHooks::screenDepth() const
{
    static int depth = qgetenv("QT_QPA_EGLFS_DEPTH").toInt();

    if (depth == 0) {
        struct fb_var_screeninfo vinfo;

        if (framebuffer != -1) {
            if (ioctl(framebuffer, FBIOGET_VSCREENINFO, &vinfo) == -1)
                qWarning("EGLFS: Could not query variable screen info.");
            else
                depth = vinfo.bits_per_pixel;
        }

        const int defaultDepth = 32;

        if (depth <= 0) {
            depth = defaultDepth;

            qWarning("EGLFS: Unable to query screen depth, defaulting to %d.\n"
                     "EGLFS: To override, set QT_QPA_EGLFS_DEPTH.", defaultDepth);
        }
    }

    return depth;
}

QImage::Format QEglFSHooks::screenFormat() const
{
    return screenDepth() == 16 ? QImage::Format_RGB16 : QImage::Format_RGB32;
}

QSurfaceFormat QEglFSHooks::surfaceFormatFor(const QSurfaceFormat &inputFormat) const
{
    QSurfaceFormat newFormat = inputFormat; 
    QByteArray depthString = qgetenv("QT_QPA_EGLFS_DEPTH");
    if (depthString.toInt() == 16) {
        newFormat.setRedBufferSize(5);
        newFormat.setGreenBufferSize(6);
        newFormat.setBlueBufferSize(5);
    } else {
        newFormat.setStencilBufferSize(8);
	newFormat.setAlphaBufferSize(8);
        newFormat.setRedBufferSize(8);
        newFormat.setGreenBufferSize(8);
        newFormat.setBlueBufferSize(8);
    }

    return newFormat;
}

bool QEglFSHooks::filterConfig(EGLDisplay, EGLConfig) const
{
    return true;
}

EGLNativeWindowType QEglFSHooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    hwc_win = new HWComposerNativeWindow(size.width(), size.height(), HAL_PIXEL_FORMAT_RGBA_8888);

    size_t neededsize = sizeof(hwc_display_contents_1_t) + 2 * sizeof(hwc_layer_1_t);
    hwc_list = (hwc_display_contents_1_t *) malloc(neededsize);
    hwc_mList = (hwc_display_contents_1_t **) malloc(HWC_NUM_DISPLAY_TYPES * sizeof(hwc_display_contents_1_t *));
    const hwc_rect_t r = { 0, 0, size.width(), size.height() };

    int counter = 0;
    for (; counter < HWC_NUM_DISPLAY_TYPES; counter++)
	    hwc_mList[counter] = hwc_list;

    hwc_layer_1_t *layer = &hwc_list->hwLayers[0];
    memset(layer, 0, sizeof(hwc_layer_1_t));
    layer->compositionType = HWC_FRAMEBUFFER;
    layer->hints = 0;
    layer->flags = 0;
    layer->handle = 0;
    layer->transform = 0;
    layer->blending = HWC_BLENDING_PREMULT;
    layer->sourceCrop = r;
    layer->displayFrame = r;
    layer->visibleRegionScreen.numRects = 1;
    layer->visibleRegionScreen.rects = &layer->displayFrame;
    layer->acquireFenceFd = -1;
    layer->releaseFenceFd = -1;
    layer = &hwc_list->hwLayers[1];
    memset(layer, 0, sizeof(hwc_layer_1_t));
    layer->compositionType = HWC_FRAMEBUFFER_TARGET;
    layer->hints = 0;
    layer->flags = 0;
    layer->handle = 0;
    layer->transform = 0;
    layer->blending = HWC_BLENDING_NONE;
    layer->sourceCrop = r;
    layer->displayFrame = r;
    layer->visibleRegionScreen.numRects = 1;
    layer->visibleRegionScreen.rects = &layer->displayFrame;
    layer->acquireFenceFd = -1;
    layer->releaseFenceFd = -1;

    hwc_list->retireFenceFd = -1;
    hwc_list->flags = HWC_GEOMETRY_CHANGED;
    hwc_list->numHwLayers = 2;

    return (EGLNativeWindowType) static_cast<ANativeWindow *>(hwc_win);
}

void QEglFSHooks::destroyNativeWindow(EGLNativeWindowType window)
{
    Q_UNUSED(window);
}

bool QEglFSHooks::hasCapability(QPlatformIntegration::Capability cap) const
{
    Q_UNUSED(cap);
    return false;
}

QEglFSCursor *QEglFSHooks::createCursor(QEglFSScreen *screen) const
{
    return new QEglFSCursor(screen);
}

void QEglFSHooks::waitForVSync() const
{
#if defined(FBIO_WAITFORVSYNC)
    static const bool forceSync = qgetenv("QT_QPA_EGLFS_FORCEVSYNC").toInt();
    if (forceSync && framebuffer != -1) {
        int arg = 0;
        if (ioctl(framebuffer, FBIO_WAITFORVSYNC, &arg) == -1)
            qWarning("Could not wait for vsync.");
    }
#endif
}

void QEglFSHooks::postSwap() const
{
	ANativeWindowBuffer *front, *back;
        hwc_win->lockBuffers(&front, &back);

	hwc_mList[0]->hwLayers[0].handle = front->handle;
	hwc_mList[0]->hwLayers[1].handle = back->handle;
	oldretire = hwc_mList[0]->retireFenceFd;
	oldrelease = hwc_mList[0]->hwLayers[0].releaseFenceFd;
	int err = hwcDevicePtr->prepare(hwcDevicePtr, HWC_NUM_DISPLAY_TYPES, hwc_mList);
	assert(err == 0);

	assert(hwc_mList[0]->hwLayers[0].compositionType == HWC_OVERLAY);

	err = hwcDevicePtr->set(hwcDevicePtr, HWC_NUM_DISPLAY_TYPES, hwc_mList);
	assert(err == 0);

	if (oldrelease != -1)
	{
		sync_wait(oldrelease, -1);
		close(oldrelease);
	}
	if (oldretire != -1)
	{
		sync_wait(oldretire, -1);
		close(oldretire);
	}
} 

#ifndef EGLFS_PLATFORM_HOOKS
QEglFSHooks stubHooks;
#endif

QT_END_NAMESPACE
