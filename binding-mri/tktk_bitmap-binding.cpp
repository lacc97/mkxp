#include "binding-util.h"

#include "bitmap.h"

#define WIN32_API extern "C" __attribute__ ((visibility ("default")))

namespace {
  Bitmap* fromId(VALUE id)
  {
    auto* rdata = reinterpret_cast<RData*>(id*2);
    return static_cast<Bitmap*>(rdata->data);
  }
}

WIN32_API int tktk_bitmap_BlendBlt(VALUE d_id, int x, int y, VALUE s_id, int r_x, int r_y, int r_width, int r_height,
                                   int blend_type, int opacity)
{
  auto* d_bm = fromId(d_id);
  auto* s_bm = fromId(s_id);

  if(d_bm && s_bm)
    d_bm->blendBlt(x, y, *s_bm, IntRect{r_x, r_y, r_width, r_height}, blend_type, opacity);

  return 0;
}