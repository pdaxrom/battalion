/*
 * Copyright (c) 2026 Battalion contributors.
 */
#import <MetalKit/MetalKit.h>

#include "backend.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FRONTEND_DEPTH_BUFFER_BIT 0x00000100u
#define FRONTEND_COLOR_BUFFER_BIT 0x00004000u
#define MAX_TEXTURES 256

typedef struct MetalUniforms {
    float mvp[16];
    uint32_t textured;
    float pointSize;
    uint32_t padding[2];
} MetalUniforms;

static MTKView *renderView;
static id<MTLDevice> device;
static id<MTLCommandQueue> commandQueue;
static id<MTLCommandBuffer> commandBuffer;
static id<MTLRenderCommandEncoder> encoder;
static id<CAMetalDrawable> drawable;
static id<MTLRenderPipelineState> opaquePipeline;
static id<MTLRenderPipelineState> blendPipeline;
static id<MTLDepthStencilState> depthEnabledState;
static id<MTLDepthStencilState> depthReadOnlyState;
static id<MTLDepthStencilState> depthDisabledState;
static id<MTLSamplerState> samplers[4];
static id<MTLTexture> textures[MAX_TEXTURES];
static unsigned int nextTexture = 1;
static unsigned int whiteTexture;
static float defaultClearColor[4] = {0, 0, 0, 1};

static id<MTLRenderPipelineState> makePipeline(id<MTLLibrary> library, BOOL blending)
{
    MTLRenderPipelineDescriptor *descriptor;
    MTLVertexDescriptor *vertexDescriptor;
    MTLRenderPipelineColorAttachmentDescriptor *color;
    NSError *error = nil;
    id<MTLRenderPipelineState> pipeline;

    descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.vertexFunction = [library newFunctionWithName:@"battalionVertex"];
    descriptor.fragmentFunction = [library newFunctionWithName:@"battalionFragment"];
    descriptor.colorAttachments[0].pixelFormat = renderView.colorPixelFormat;
    descriptor.depthAttachmentPixelFormat = renderView.depthStencilPixelFormat;

    vertexDescriptor = [[MTLVertexDescriptor alloc] init];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[0].offset = offsetof(RendererVertex, position);
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[1].offset = offsetof(RendererVertex, color);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.attributes[2].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[2].offset = offsetof(RendererVertex, texcoord);
    vertexDescriptor.attributes[2].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = sizeof(RendererVertex);
    descriptor.vertexDescriptor = vertexDescriptor;

    color = descriptor.colorAttachments[0];
    color.blendingEnabled = blending;
    color.rgbBlendOperation = MTLBlendOperationAdd;
    color.alphaBlendOperation = MTLBlendOperationAdd;
    color.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    color.sourceAlphaBlendFactor = MTLBlendFactorOne;
    color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:&error];
    if (!pipeline)
        fprintf(stderr, "Cannot create Metal pipeline: %s\n", error.localizedDescription.UTF8String);
    return pipeline;
}

static void createDepthStates(void)
{
    MTLDepthStencilDescriptor *descriptor;

    descriptor = [[MTLDepthStencilDescriptor alloc] init];
    descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
    descriptor.depthWriteEnabled = YES;
    depthEnabledState = [device newDepthStencilStateWithDescriptor:descriptor];

    descriptor.depthWriteEnabled = NO;
    depthReadOnlyState = [device newDepthStencilStateWithDescriptor:descriptor];

    descriptor = [[MTLDepthStencilDescriptor alloc] init];
    descriptor.depthCompareFunction = MTLCompareFunctionAlways;
    descriptor.depthWriteEnabled = NO;
    depthDisabledState = [device newDepthStencilStateWithDescriptor:descriptor];
}

static void createSamplers(void)
{
    int repeat, linear;
    for (repeat = 0; repeat < 2; ++repeat) {
        for (linear = 0; linear < 2; ++linear) {
            MTLSamplerDescriptor *descriptor = [[MTLSamplerDescriptor alloc] init];
            descriptor.minFilter = linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
            descriptor.magFilter = linear ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
            descriptor.sAddressMode = repeat ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
            descriptor.tAddressMode = repeat ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
            samplers[repeat * 2 + linear] = [device newSamplerStateWithDescriptor:descriptor];
        }
    }
}

static int beginFrame(unsigned int buffers, const float color[4], float depth)
{
    MTLRenderPassDescriptor *pass;
    if (encoder)
        return 1;

    drawable = renderView.currentDrawable;
    pass = renderView.currentRenderPassDescriptor;
    if (!drawable || !pass)
        return 0;

    commandBuffer = [commandQueue commandBuffer];
    pass.colorAttachments[0].loadAction = (buffers & FRONTEND_COLOR_BUFFER_BIT)
        ? MTLLoadActionClear : MTLLoadActionDontCare;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(color[0], color[1], color[2], color[3]);
    if (pass.depthAttachment) {
        pass.depthAttachment.loadAction = (buffers & FRONTEND_DEPTH_BUFFER_BIT)
            ? MTLLoadActionClear : MTLLoadActionDontCare;
        pass.depthAttachment.storeAction = MTLStoreActionDontCare;
        pass.depthAttachment.clearDepth = depth;
    }
    encoder = [commandBuffer renderCommandEncoderWithDescriptor:pass];
    return encoder != nil;
}

int rendererBackendInitialize(void *nativeView)
{
    static NSString *source =
        @"#include <metal_stdlib>\n"
         "using namespace metal;\n"
         "struct VertexIn { float3 position [[attribute(0)]]; float4 color [[attribute(1)]]; float2 uv [[attribute(2)]]; };\n"
         "struct VertexOut { float4 position [[position]]; float4 color; float2 uv; float pointSize [[point_size]]; };\n"
         "struct Uniforms { float4x4 mvp; uint textured; float pointSize; uint2 padding; };\n"
         "vertex VertexOut battalionVertex(VertexIn in [[stage_in]], constant Uniforms &u [[buffer(1)]]) {\n"
         "  VertexOut out; float4 p = u.mvp * float4(in.position, 1.0); p.z = (p.z + p.w) * 0.5;\n"
         "  out.position = p; out.color = in.color; out.uv = in.uv; out.pointSize = max(u.pointSize, 1.0); return out; }\n"
         "fragment float4 battalionFragment(VertexOut in [[stage_in]], constant Uniforms &u [[buffer(1)]],\n"
         "  texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
         "  if (u.textured != 0) { return in.color * tex.sample(smp, in.uv); }\n"
         "  return in.color; }\n";
    NSError *error = nil;
    id<MTLLibrary> library;
    unsigned char white = 255;

    renderView = (__bridge MTKView *)nativeView;
    device = renderView.device;
    commandQueue = [device newCommandQueue];
    library = [device newLibraryWithSource:source options:nil error:&error];
    if (!library) {
        fprintf(stderr, "Cannot compile Metal shaders: %s\n", error.localizedDescription.UTF8String);
        return 0;
    }
    opaquePipeline = makePipeline(library, NO);
    blendPipeline = makePipeline(library, YES);
    createDepthStates();
    createSamplers();
    whiteTexture = rendererBackendCreateLuminanceTexture(1, 1, &white);
    return opaquePipeline && blendPipeline && commandQueue;
}

void rendererBackendResize(int width, int height)
{
    renderView.drawableSize = CGSizeMake(width, height);
}

void rendererBackendShutdown(void)
{
    int i;
    if (encoder)
        [encoder endEncoding];
    encoder = nil;
    commandBuffer = nil;
    drawable = nil;
    for (i = 0; i < MAX_TEXTURES; ++i)
        textures[i] = nil;
    renderView = nil;
    device = nil;
}

void rendererBackendClear(unsigned int buffers, const float color[4], float depth)
{
    memcpy(defaultClearColor, color, sizeof(defaultClearColor));
    beginFrame(buffers, color, depth);
}

void rendererBackendDraw(enum RendererPrimitive primitive,
                         const RendererVertex *vertices,
                         size_t vertexCount,
                         const RendererDrawState *state)
{
    id<MTLBuffer> vertexBuffer;
    MetalUniforms uniforms;
    MTLViewport viewport;
    MTLScissorRect scissor;
    MTLPrimitiveType metalPrimitive;
    CGFloat scaleX, scaleY;
    unsigned int texture;
    int samplerIndex;

    if (!vertexCount || !beginFrame(FRONTEND_COLOR_BUFFER_BIT | FRONTEND_DEPTH_BUFFER_BIT,
                                    defaultClearColor, 1.0f))
        return;

    vertexBuffer = [device newBufferWithBytes:vertices
                                        length:vertexCount * sizeof(RendererVertex)
                                       options:MTLResourceStorageModeShared];
    memcpy(uniforms.mvp, state->mvp, sizeof(uniforms.mvp));
    uniforms.textured = state->textureEnabled && state->texture != 0;
    uniforms.pointSize = state->pointSize;
    memset(uniforms.padding, 0, sizeof(uniforms.padding));

    [encoder setRenderPipelineState:state->blending ? blendPipeline : opaquePipeline];
    [encoder setDepthStencilState:state->depthTest
        ? (state->depthWrite ? depthEnabledState : depthReadOnlyState)
        : depthDisabledState];
    [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
    [encoder setCullMode:state->cullBackFaces ? MTLCullModeBack : MTLCullModeNone];
    [encoder setTriangleFillMode:state->wireframe ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
    [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
    [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];

    texture = uniforms.textured ? state->texture : whiteTexture;
    if (texture >= MAX_TEXTURES || !textures[texture])
        texture = whiteTexture;
    samplerIndex = (state->textureRepeat ? 2 : 0) + (state->textureLinear ? 1 : 0);
    [encoder setFragmentTexture:textures[texture] atIndex:0];
    [encoder setFragmentSamplerState:samplers[samplerIndex] atIndex:0];

    scaleX = renderView.bounds.size.width > 0.0
        ? renderView.drawableSize.width / renderView.bounds.size.width : 1.0;
    scaleY = renderView.bounds.size.height > 0.0
        ? renderView.drawableSize.height / renderView.bounds.size.height : 1.0;
    viewport.originX = state->viewport[0] * scaleX;
    viewport.originY = renderView.drawableSize.height
        - (state->viewport[1] + state->viewport[3]) * scaleY;
    viewport.width = state->viewport[2] * scaleX;
    viewport.height = state->viewport[3] * scaleY;
    viewport.znear = 0.0;
    viewport.zfar = 1.0;
    if (viewport.width > 0 && viewport.height > 0)
        [encoder setViewport:viewport];

    if (state->scissorEnabled) {
        int left = (int)(state->scissor[0] * scaleX);
        int bottom = (int)(state->scissor[1] * scaleY);
        int right = (int)((state->scissor[0] + state->scissor[2]) * scaleX);
        int top = (int)((state->scissor[1] + state->scissor[3]) * scaleY);
        int drawableWidth = (int)renderView.drawableSize.width;
        int drawableHeight = (int)renderView.drawableSize.height;
        if (left < 0) left = 0;
        if (bottom < 0) bottom = 0;
        if (right > drawableWidth) right = drawableWidth;
        if (top > drawableHeight) top = drawableHeight;
        if (right < left) right = left;
        if (top < bottom) top = bottom;
        scissor.x = (NSUInteger)left;
        scissor.y = (NSUInteger)(drawableHeight - top);
        scissor.width = (NSUInteger)(right - left);
        scissor.height = (NSUInteger)(top - bottom);
    } else {
        scissor.x = 0;
        scissor.y = 0;
        scissor.width = (NSUInteger)renderView.drawableSize.width;
        scissor.height = (NSUInteger)renderView.drawableSize.height;
    }
    if (scissor.width == 0 || scissor.height == 0)
        return;
    [encoder setScissorRect:scissor];

    metalPrimitive = primitive == RENDERER_LINES ? MTLPrimitiveTypeLine
        : primitive == RENDERER_POINTS ? MTLPrimitiveTypePoint : MTLPrimitiveTypeTriangle;
    [encoder drawPrimitives:metalPrimitive vertexStart:0 vertexCount:vertexCount];
}

unsigned int rendererBackendCreateLuminanceTexture(int width, int height,
                                                    const unsigned char *pixels)
{
    MTLTextureDescriptor *descriptor;
    MTLRegion region;
    unsigned int texture;

    if (!device || nextTexture >= MAX_TEXTURES || width <= 0 || height <= 0)
        return 0;
    texture = nextTexture++;
    descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                                                    width:width
                                                                   height:height
                                                                mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
    textures[texture] = [device newTextureWithDescriptor:descriptor];
    region = MTLRegionMake2D(0, 0, width, height);
    [textures[texture] replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:width];
    return texture;
}

unsigned int rendererBackendCreateRGBA8Texture(int width, int height,
                                                const unsigned char *pixels)
{
    MTLTextureDescriptor *descriptor;
    MTLRegion region;
    unsigned int texture;

    if (!device || nextTexture >= MAX_TEXTURES || width <= 0 || height <= 0)
        return 0;
    texture = nextTexture++;
    descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                    width:width
                                                                   height:height
                                                                mipmapped:NO];
    descriptor.usage = MTLTextureUsageShaderRead;
    textures[texture] = [device newTextureWithDescriptor:descriptor];
    region = MTLRegionMake2D(0, 0, width, height);
    [textures[texture] replaceRegion:region mipmapLevel:0 withBytes:pixels bytesPerRow:width * 4];
    return texture;
}

void rendererBackendPresent(void)
{
    if (!encoder)
        return;
    [encoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
    encoder = nil;
    commandBuffer = nil;
    drawable = nil;
}
