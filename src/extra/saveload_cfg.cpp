#include "saveload_cfg.h"
#include <cstring>
#include "runtime_services.h"



namespace freegag
{

constexpr char saveload_cfg_data[] = R"([CFG]
fademask=0;
flags=NOSAVE;
mouse=CM /FILE:K_Ukaz.bmp /F:NOPAL;
mouse=EXM /FILE:K_exit.256:0 /FILE:K_exit.bmp:1 /F:NOPAL;
command=Comment /MOUSE:CM;
command=Go;
object=SL INIT::OFF EDITING::OFF EMPTY::OFF CLOSE::OFF;
object=INPUT NAME::X;

[LOAD]
flags=NOSAVE NOCOMMENT;
sublocation=COMMON;
sublocation=TAG_LOAD;
image=HW_1 /FILE::DARK0.BMP /F:PRIMARY;
zone=z_CAPTION /POS::168,366,304,28 /COMM:Comment /MOUSE:CM /P:100;
event=e_PREVIEW /ZONE::z_PREVIEW /COMM:Comment /MESSAGE::2102;
event=e_EXIT_LOAD /ZONE::z_MAIN /COMM:Go /PEXIT:NOFADE;

[SAVE]
flags=NOSAVE NOCOMMENT;
sublocation=COMMON;
sublocation=TAG_SAVE;
local=l_INIT SL::INIT::OFF;
local=l_SAVE_NOT_EDITING SL::EDITING::OFF;
image=HW_1 /FILE::DARK0.BMP /F:PRIMARY;
zone=z_NAME /POS::168,366,304,28 /COMM:Comment /MOUSE:CM /P:100;
event=e_NAME /ZONE::z_NAME /C:l_SAVE_NOT_EDITING /COMM:Comment /SET::SL:EDITING:ON /MESSAGE::2103 /INPSTR:168:366:SaveCaption:208:96:INPUT:NAME:16 /MESSAGE::2104 /SET::SL:EDITING:OFF /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;
event=e_INIT /C:l_INIT /SET::SL:INIT:ON /MESSAGE::2105 /SWVALUE::SL::EMPTY /VALUE::ON /GOTO::e_INIT_INPUT /BREAK /CSEND /GOTO::e_INIT_DONE /LABEL::e_INIT_INPUT /SET::SL:EDITING:ON /MESSAGE::2103 /INPSTR:168:366:SaveCaption:208:96:INPUT:NAME:16 /MESSAGE::2104 /SET::SL:EDITING:OFF /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND /LABEL::e_INIT_DONE;
event=e_EXIT_SAVE /ZONE::z_MAIN /COMM:Go /MESSAGE::2106 /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;

[COMMON]
zone=z_MAIN /RECT::0,0,640,480 /MOUSE:EXM /COMM:Go;
zone=z_PREVIEW /POS::160,120,320,240 /COMM:Comment /MOUSE:CM /P:100;
zone=z_DETAILS /POS::160,360,320,48 /COMM:Comment /MOUSE:CM /P:99;
zone=z_CONTROLS /POS::232,408,176,48 /COMM:Comment /MOUSE:CM /P:99;
zone=z_BACK /POS::232,408,48,48 /COMM:Comment /MOUSE:CM /P:100;
zone=z_NEXT /POS::360,408,48,48 /COMM:Comment /MOUSE:CM /P:100;
image=i_BACK /FILE::SL-LEFT.BMP /POS::232,408 /F:NOPAL;
image=i_NEXT /FILE::SL-RIGHT.BMP /POS::360,408 /F:NOPAL;
font=SaveCaption /FILE:Font2.rus;
layer=SavePreview /POS:160,120,320,240 /Z:458752;
layer=SaveCaption /POS:168,366,304,28 /Z:458753;
event=e_BACK /ZONE::z_BACK /COMM:Comment /MESSAGE::2100;
event=e_NEXT /ZONE::z_NEXT /COMM:Comment /MESSAGE::2101;

[TAG_LOAD]
zone=z_LOAD /POS::296,408,48,48 /COMM:Comment /MOUSE:CM /P:100;
image=i_LOAD /FILE::SL-LOAD.BMP /POS::296,408 /F:NOPAL;
event=e_LOAD /ZONE::z_LOAD /COMM:Comment /MESSAGE::2102;

[TAG_SAVE]
zone=z_SAVE /POS::296,408,48,48 /COMM:Comment /MOUSE:CM /P:100;
image=i_SAVE /FILE::SL-SAVE.BMP /POS::296,408 /F:NOPAL;
event=e_SAVE /ZONE::z_SAVE /COMM:Comment /MESSAGE::2102 /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;

[END]
)";

std::pair<void *, uint32_t> synthesize_saveload_cfg(RuntimeHeap *heap)
{
    constexpr uint32_t size = sizeof(saveload_cfg_data) - 1;
    void *data = allocate_runtime_heap(heap, 0, size);
    if(data == nullptr)
        return {};
    std::memcpy(data, saveload_cfg_data, size);
    return { data, size };
}

}
