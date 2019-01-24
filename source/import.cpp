#include <3ds.h>
#include <fstream>
#include "import.hpp"
#include "io.hpp"
#include "STDirectory.hpp"
#include "streetpass/BoxInfo.hpp"
#include "streetpass/Box.hpp"
#include "streetpass/MBoxList.hpp"

extern "C"
{
#include "3ds/services/cecdu.h"
}

using Streetpass::Box;
using Streetpass::MBoxListHeader;
using Streetpass::Message;

void importBoxes(bool del)
{
    //Result res; currently unused
    MBoxListHeader list;
    u32 size = sizeof(MBoxListHeader);
    CECDU_Open(0x0, CEC_PATH_MBOX_LIST, CEC_READ | CEC_CHECK, nullptr);
    CECDU_Read(size, &list, nullptr);
    STDirectory dir("/3ds/CECTool");
    std::vector<std::string> boxes;
    for (size_t i = 0; i < dir.count(); i++)
    {
        if (dir.folder(i))
        {
            if (io::folderExists("/3ds/CECTool/" + dir.item(i) + "/InBox___"))
            {
                if (dir.item(i).find_first_not_of("1234567890abcdef") == std::string::npos)
                {
                    boxes.push_back(dir.item(i));
                }
            }
        }
    }
    std::vector<int> availableBoxes;
    for (u32 i = 0; i < list.numBoxes; i++)
    {
        if (std::find(boxes.begin(), boxes.end(), list.boxIds[i]) != boxes.end())
        {
            availableBoxes.push_back(i);
        }
    }
    if (availableBoxes.size() > 12)
    {
        printf("Too many boxes!\nI'm not implementing box choices\nin this limited beta.\nWait for a UI, please\n");
        return;
    }
    else
    {
        u32 currentId;
        std::string boxId;
        for (size_t i = 0; i < availableBoxes.size(); i++)
        {
            boxId = list.boxIds[availableBoxes[i]];
            currentId = std::stoul(list.boxIds[availableBoxes[i]], nullptr, 16);
            dir = STDirectory("/3ds/CECTool/" + boxId + "/InBox___");
            Box box(currentId, false);
            for (size_t j = 0; j < dir.good() && dir.count() && box.getMessages().size() < box.getInfo().maxMessages(); j++)
            {
                FILE* in = fopen(("/3ds/CECTool/" + boxId + "/InBox___/" + dir.item(j)).c_str(), "r");
                fseek(in, 0, SEEK_END);
                size_t messageSize = ftell(in);
                fseek(in, 0, SEEK_SET);
                std::vector<u8> buffer(messageSize);
                fread(buffer.data(), 1, messageSize, in);
                fclose(in);

                Message message(buffer);
                box.addMessage(message);
                if (del)
                {
                    remove(("/3ds/CECTool/" + boxId + "/InBox___/" + dir.item(j)).c_str());
                }
            }

            dir = STDirectory("/3ds/CECTool/" + boxId + "/OutBox__");
            for (size_t j = 0; dir.good() && j < dir.count() && box.getMessages().size() < box.getInfo().maxMessages(); j++)
            {
                FILE* in = fopen(("/3ds/CECTool/" + boxId + "/OutBox__/" + dir.item(j)).c_str(), "r");
                fseek(in, 0, SEEK_END);
                size_t messageSize = ftell(in);
                fseek(in, 0, SEEK_SET);
                std::vector<u8> buffer(messageSize);
                fread(buffer.data(), 1, messageSize, in);
                fclose(in);

                Message message(buffer);
                message.getInfo().updateTimes();
                box.addMessage(message);
                if (del)
                {
                    remove(("/3ds/CECTool/" + boxId + "/OutBox__/" + dir.item(j)).c_str());
                }
            }

            if (del)
            {
                remove(("/3ds/CECTool/" + boxId).c_str());
            }
            box.saveInfo();
        }
    }
}