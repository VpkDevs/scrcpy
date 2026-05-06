/*
 * DXGI Desktop Duplication — primary monitor that is AttachedToDesktop.
 */

#include "duplex_capture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITGUID
#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>

static ID3D11Device *g_dev;
static ID3D11DeviceContext *g_ctx;
static IDXGIOutputDuplication *g_dup;
static ID3D11Texture2D *g_staging;
static UINT g_width;
static UINT g_height;

bool
duplex_dxgi_init(void) {
    g_dev = NULL;
    g_ctx = NULL;
    g_dup = NULL;
    g_staging = NULL;
    g_width = 0;
    g_height = 0;

    IDXGIFactory1 *factory = NULL;
    HRESULT hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **) &factory);
    if (FAILED(hr)) {
        fprintf(stderr, "duplex_dxgi: CreateDXGIFactory1 failed 0x%lx\n",
                (unsigned long) hr);
        return false;
    }

    IDXGIAdapter1 *adapter = NULL;
    IDXGIOutput *output = NULL;
    bool found = false;

    for (UINT ai = 0;
         factory->lpVtbl->EnumAdapters1(factory, ai, &adapter) != DXGI_ERROR_NOT_FOUND;
         ++ai) {
        for (UINT oi = 0;
             adapter->lpVtbl->EnumOutputs(adapter, oi, &output) !=
                     DXGI_ERROR_NOT_FOUND;
             ++oi) {
            DXGI_OUTPUT_DESC od;
            output->lpVtbl->GetDesc(output, &od);
            if (!od.AttachedToDesktop) {
                output->lpVtbl->Release(output);
                output = NULL;
                continue;
            }
            found = true;
            break;
        }
        if (found) {
            break;
        }
        adapter->lpVtbl->Release(adapter);
        adapter = NULL;
    }

    if (!found || !output || !adapter) {
        fprintf(stderr, "duplex_dxgi: no desktop output found\n");
        if (output) {
            output->lpVtbl->Release(output);
        }
        if (adapter) {
            adapter->lpVtbl->Release(adapter);
        }
        factory->lpVtbl->Release(factory);
        return false;
    }

    D3D_FEATURE_LEVEL fl_out;
    hr = D3D11CreateDevice((IDXGIAdapter *) adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0,
                           D3D11_SDK_VERSION, &g_dev, &fl_out, &g_ctx);
    if (FAILED(hr)) {
        fprintf(stderr, "duplex_dxgi: D3D11CreateDevice failed 0x%lx\n",
                (unsigned long) hr);
        output->lpVtbl->Release(output);
        adapter->lpVtbl->Release(adapter);
        factory->lpVtbl->Release(factory);
        return false;
    }

    IDXGIOutput1 *out1 = NULL;
    hr = output->lpVtbl->QueryInterface(output, &IID_IDXGIOutput1,
                                        (void **) &out1);
    output->lpVtbl->Release(output);
    output = NULL;
    if (FAILED(hr) || !out1) {
        fprintf(stderr, "duplex_dxgi: QueryInterface IDXGIOutput1 failed\n");
        g_ctx->lpVtbl->Release(g_ctx);
        g_dev->lpVtbl->Release(g_dev);
        g_ctx = NULL;
        g_dev = NULL;
        adapter->lpVtbl->Release(adapter);
        factory->lpVtbl->Release(factory);
        return false;
    }

    hr = out1->lpVtbl->DuplicateOutput(out1, g_dev, &g_dup);
    out1->lpVtbl->Release(out1);
    adapter->lpVtbl->Release(adapter);
    factory->lpVtbl->Release(factory);

    if (FAILED(hr) || !g_dup) {
        fprintf(stderr, "duplex_dxgi: DuplicateOutput failed 0x%lx\n",
                (unsigned long) hr);
        g_ctx->lpVtbl->Release(g_ctx);
        g_dev->lpVtbl->Release(g_dev);
        g_ctx = NULL;
        g_dev = NULL;
        return false;
    }

    DXGI_OUTDUPL_DESC ddesc;
    g_dup->lpVtbl->GetDesc(g_dup, &ddesc);
    g_width = ddesc.ModeDesc.Width;
    g_height = ddesc.ModeDesc.Height;

    D3D11_TEXTURE2D_DESC td = {0};
    td.Width = g_width;
    td.Height = g_height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    td.MiscFlags = 0;

    hr = g_dev->lpVtbl->CreateTexture2D(g_dev, &td, NULL, &g_staging);
    if (FAILED(hr)) {
        fprintf(stderr, "duplex_dxgi: CreateTexture2D staging failed 0x%lx\n",
                (unsigned long) hr);
        g_dup->lpVtbl->Release(g_dup);
        g_dup = NULL;
        g_ctx->lpVtbl->Release(g_ctx);
        g_dev->lpVtbl->Release(g_dev);
        g_ctx = NULL;
        g_dev = NULL;
        return false;
    }

    fprintf(stderr, "duplex_dxgi: %ux%u\n", g_width, g_height);
    return true;
}

void
duplex_dxgi_shutdown(void) {
    if (g_staging) {
        g_staging->lpVtbl->Release(g_staging);
        g_staging = NULL;
    }
    if (g_dup) {
        g_dup->lpVtbl->Release(g_dup);
        g_dup = NULL;
    }
    if (g_ctx) {
        g_ctx->lpVtbl->Release(g_ctx);
        g_ctx = NULL;
    }
    if (g_dev) {
        g_dev->lpVtbl->Release(g_dev);
        g_dev = NULL;
    }
    g_width = g_height = 0;
}

uint32_t
duplex_dxgi_width(void) {
    return g_width;
}

uint32_t
duplex_dxgi_height(void) {
    return g_height;
}

bool
duplex_dxgi_grab_frame(uint8_t *out_bgra, size_t out_size) {
    if (!g_dup || !g_staging || !out_bgra) {
        return false;
    }

    size_t need = (size_t) g_width * (size_t) g_height * 4;
    if (out_size < need) {
        return false;
    }

    IDXGIResource *frame = NULL;
    DXGI_OUTDUPL_FRAME_INFO fi;
    HRESULT hr = g_dup->lpVtbl->AcquireNextFrame(g_dup, 100, &fi, &frame);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        /* repeat last frame conceptually — duplicate empty; caller may skip send */
        return false;
    }
    if (FAILED(hr) || !frame) {
        return false;
    }

    ID3D11Texture2D *tex = NULL;
    hr = frame->lpVtbl->QueryInterface(frame, &IID_ID3D11Texture2D,
                                         (void **) &tex);
    frame->lpVtbl->Release(frame);
    if (FAILED(hr) || !tex) {
        g_dup->lpVtbl->ReleaseFrame(g_dup);
        return false;
    }

    g_ctx->lpVtbl->CopyResource(g_ctx, (ID3D11Resource *) g_staging,
                                (ID3D11Resource *) tex);
    tex->lpVtbl->Release(tex);

    D3D11_MAPPED_SUBRESOURCE map;
    hr = g_ctx->lpVtbl->Map(g_ctx, (ID3D11Resource *) g_staging, 0,
                            D3D11_MAP_READ, 0, &map);
    if (FAILED(hr)) {
        g_dup->lpVtbl->ReleaseFrame(g_dup);
        return false;
    }

    uint8_t *src = (uint8_t *) map.pData;
    size_t row_copy = (size_t) g_width * 4;
    for (UINT y = 0; y < g_height; ++y) {
        memcpy(out_bgra + (size_t) y * row_copy, src + (size_t) y * map.RowPitch,
               row_copy);
    }

    g_ctx->lpVtbl->Unmap(g_ctx, (ID3D11Resource *) g_staging, 0);
    g_dup->lpVtbl->ReleaseFrame(g_dup);
    return true;
}
