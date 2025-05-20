#include "battery.hh"

#include "adt/file.hh"
#include "adt/BufferAllocator.hh"

using namespace adt;


namespace battery
{

Report
Report::read(const char* ntsPath, adt::ScratchBuffer* pScratch)
{
    file::TYPE eType = file::fileType(ntsPath);
    if (eType != file::TYPE::DIRECTORY)
    {
        print::err("Battery: path should look like '/sys/class/power_supply/BATT*', got: '{}'\n", ntsPath);
        return {};
    }

    try
    {
        BufferAllocator buffer {pScratch->nextMem<u8>()};
        defer( pScratch->reset() );

        String sStatus = file::appendDirPath(&buffer, ntsPath, "status");
        String sCapacity = file::appendDirPath(&buffer, ntsPath, "capacity");

        Report report {};

        StringFixed<64> sfStatus = file::load<64>(sStatus.data());

        if (StringView(sfStatus).contains("Charging"))
            report.eStatus = STATUS::CHARGING;
        else if (StringView(sfStatus).contains("Discharging"))
            report.eStatus = STATUS::DISCHARCHING;
        else if (StringView(sfStatus).contains("Not Charging"))
            report.eStatus = STATUS::NOT_CHARCHING;
        else if (StringView(sfStatus).contains("Full"))
            report.eStatus = STATUS::FULL;

        StringFixed<64> sfCapacity = file::load<64>(sCapacity.data());
        report.cap = atoi(sfCapacity.data());

        return report;
    }
    catch (const AllocException& ex)
    {
        return {};
        ex.printErrorMsg(stderr);
    }

    return {};
}

} /* namespace battery */
