
enum class RenderCmd : u8
{
    
};

struct GfxCommandList
{
};

struct GfxCommandAllocator
{
};

static VertexBuffer        *g_vertex_buffers  = 0;
static u32                  g_vertex_gens     = 0;

static IndexBuffer         *g_vertex_buffers  = 0;
static u32                  g_index_gens      = 0;

static PipelineStateObject *g_pipelines       = 0;
static u32                  g_pipeline_gens   = 0;

static RootSignature       *g_root_signatures = 0;
static u32                  g_root_gens       = 0;
