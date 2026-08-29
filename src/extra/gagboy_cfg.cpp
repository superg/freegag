#include "gagboy_cfg.h"
#include <cstring>
#include "runtime_services.h"



namespace freegag
{

constexpr char gagboy_cfg_data[] = R"([CFG]
event=e_START /RELOADNOFADE:GAGBOY;

[GAGBOY]
flags=NOSAVE NOCOMMENT PAL_NOADJUST;
mouse=TET /FILE:K_ghand.256:0 /FILE:K_ukaz.bmp:1 /F:NOPAL /POS:7,11;
mouse=TNM /FILE:K_gnone.256:0 /FILE:K_none.bmp:1 /F:NOPAL;
command=Go;
object=GAGBoy Score::0;
image=Background /FILE:VE-GBNEW.BMP /F:NOPAL /INVERT_NOPAL /F:PRIMARY;
zone=Global /RECT:0,0,639,479 /COMM:Go /MOUSE:TNM /P:10;
zone=Start /POS:149,49,88,84 /COMM:Go /MOUSE:TET /P:20;
zone=Action1 /POS:180,185,45,200 /COMM:Go /MOUSE:TET /P:20;
zone=Action2 /POS:505,90,45,200 /COMM:Go /MOUSE:TET /P:20;
zone=Exit /POS:478,426,66,54 /COMM:Go /MOUSE:TET /P:20;
event=e_RUN /GAME:XTETDLL.DLL:GAGBoy::Score /QUIT;

[END]
)";

std::pair<void *, uint32_t> synthesize_gagboy_cfg(RuntimeHeap *heap)
{
    constexpr uint32_t size = sizeof(gagboy_cfg_data) - 1;
    void *data = allocate_runtime_heap(heap, 0, size);
    if(data == nullptr)
        return {};
    std::memcpy(data, gagboy_cfg_data, size);
    return { data, size };
}

}
