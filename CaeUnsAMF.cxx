/****************************************************************************
 *
 * class CaeUnsAMF
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2011 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#include <stdarg.h>

#include "apiCAEP.h"
#include "apiCAEPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeWrite.h"
#include "pwpPlatform.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"
#include "CaeUnsAMF.h"



class Indent {

    enum {
        DentStep = 4
    };

    void printIndent(int extra = 0) const {
        int sz  = level_ + extra;
        while (fill_.length() < sz) {
            fill_ += fill_; // double fill buffer size
        }
        ::fprintf(fp_, "%*.*s", sz, sz, fill_.c_str());
    }


public:

    Indent() {
        ++level_;
    }

    ~Indent() {
        --level_;
    }

    void printf(int extra, const char *fmt, ...) const
    {
        printIndent(extra);
        va_list args;
        va_start(args, fmt);
        ::vfprintf(fp_, fmt, args);
        va_end(args);
    }

    void printf(const char *fmt, ...) const
    {
        printIndent();
        va_list args;
        va_start(args, fmt);
        ::vfprintf(fp_, fmt, args);
        va_end(args);
    }

    void puts(const char *str, int extra = 0) const
    {
        printIndent(extra);
        ::fputs(str, fp_);
    }

    void comment(const char *fmt, ...) const
    {
        printIndent();
        ::fputs("<!-- ", fp_);
        va_list args;
        va_start(args, fmt);
        ::vfprintf(fp_, fmt, args);
        va_end(args);
        ::fputs(" -->\n", fp_);
    }


    /*
    <metadata type='X'>; X is one of:
        Name        - The alphanumeric label of the entity, to be used by the
                      interpreter if interacting with the user.
        Description - A description of the content of the entity
        URL         - A link to an external resource relating to the entity
        Author      - Specifies the name(s) of the author(s) of the entity
        Company     - Specifying the company generating the entity
        CAD         - specifies the name of the originating CAD software and
                      version
        Revision    - specifies the revision of the entity
        Tolerance   - specifies the desired manufacturing tolerance of the
                      entity in entity's unit system
        Volume      - specifies the total volume of the entity, in the
                      entity's unit system, to be used for verification
                      (object and volume only)
    */
    void metadata(const char *mdType, const char *fmt, ...) const
    {
        printIndent();
        ::fprintf(fp_, "<metadata type='%s'>", mdType);
        va_list args;
        va_start(args, fmt);
        ::vfprintf(fp_, fmt, args);
        va_end(args);
        ::fputs("</metadata>\n", fp_);
    }


    void metadata(int extra, const char *mdType, const char *fmt,
        ...) const
    {
        printIndent(extra);
        ::fprintf(fp_, "<metadata type='%s'>", mdType);
        va_list args;
        va_start(args, fmt);
        ::vfprintf(fp_, fmt, args);
        va_end(args);
        ::fputs("</metadata>\n", fp_);
    }


    static void setFp(FILE *fp)
    {
        fp_ = fp;
    }


private:

    static FILE*        fp_;
    static int          level_; // indent level
    static std::string  fill_;  // tab chars
};
FILE*       Indent::fp_     = 0;
int         Indent::level_  = 0;
std::string Indent::fill_   = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";


//***************************************************************************
//***************************************************************************
//***************************************************************************

// format spec from:
//  http://en.wikipedia.org/wiki/Additive_Manufacturing_File_Format

static const char *AttrUnits = "units";


CaeUnsAMF::CaeUnsAMF(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL
        model, const CAEP_WRITEINFO *pWriteInfo) :
    CaeUnsPlugin(pRti, model, pWriteInfo),
    units_(CaeUnsAMF::inch)
{
}


CaeUnsAMF::~CaeUnsAMF()
{
}


bool
CaeUnsAMF::beginExport()
{
    Indent::setFp(fp());
    model_.getAttribute(AttrUnits, units_, units_);
    setProgressMajorSteps(2);
    return true;
}


PWP_BOOL
CaeUnsAMF::write()
{
    fputs("<?xml version='1.0' encoding='UTF-8'?>\n", fp());
    fprintf(fp(), "<amf unit='%s'>\n", unitName()); {
        Indent dent;
        dent.metadata("Company", "Pointwise, Inc.");
        dent.metadata("URL", "http://www.pointwise.com");
        dent.metadata("CAD", "Pointwise %s", "v17");
    }
    writeObject(0);
    fputs("</amf>\n", fp());

    return true;
}


void
CaeUnsAMF::writeObject(PWP_UINT id) const
{
    Indent dent;
    dent.printf("<object id='%lu'>\n", (unsigned long)id);
    dent.metadata(1, "Name", "Object_%05lu", (unsigned long)id);
    writeMesh();
    dent.puts("</object>\n");
}


void
CaeUnsAMF::writeMesh() const
{
    Indent dent;
    dent.puts("<mesh>\n");
    writeVertices(); // major step 1
    writeVolume();   // major step 2
    dent.puts("</mesh>\n");
}


void
CaeUnsAMF::writeVertices() const
{
    if (progressBeginStep(model_.vertexCount())) {
        Indent dent;
        dent.comment("vertex count: %lu",
            (unsigned long)model_.vertexCount());
        dent.puts("<vertices>\n");
        CaeUnsVertex v(model_);
        while (v.isValid() && progressIncrement()) {
            writeVertex(v);
            ++v;
        }
        dent.puts("</vertices>\n");
    }
}


void
CaeUnsAMF::writeVertex(const CaeUnsVertex &v) const
{
    Indent dent;
    dent.puts("<vertex>\n");
    writeCoordinates(v);
    dent.puts("</vertex>\n");
}


void
CaeUnsAMF::writeCoordinates(const CaeUnsVertex &v) const
{
    Indent dent;
    dent.puts("<coordinates>\n");
    dent.printf(1, "<x>%.12f</x>\n", (double)v.x());
    dent.printf(1, "<y>%.12f</y>\n", (double)v.y());
    dent.printf(1, "<z>%.12f</z>\n", (double)v.z());
    dent.puts("</coordinates>\n");
}


void
CaeUnsAMF::writeVolume() const
{
    if (isDimension2D()) {
        writeVolume2();
    }
    else {
        writeVolume3();
    }
}


void
CaeUnsAMF::writeVolume2() const
{
    PWGM_ELEMCOUNTS ecs;
    if (progressBeginStep(model_.elementCount(&ecs))) {
        Indent dent;
        unsigned long triCnt = PWGM_ECNT_Tri(ecs) + PWGM_ECNT_Quad(ecs) * 2;
        dent.comment("triangle count: %lu", triCnt);
        dent.puts("<volume>\n");
        dent.metadata(1, "Name", "Volume_%05lu", (unsigned long)0);
        CaeUnsElement elem(model_);
        PWGM_ELEMDATA eData;
        while (elem.isValid() && elem.data(eData) && progressIncrement()) {
            writeElement(eData);
            ++elem;
        }
        dent.puts("</volume>\n");
    }
}


void
CaeUnsAMF::writeVolume3() const
{
    PWGM_ELEMCOUNTS tmp;
    PWGM_ELEMCOUNTS ecs = { {0} };
    CaeUnsPatch patch(model_);
    // tally the surface patch element counts
    while (patch.isValid()) {
        patch.elementCount(&tmp);
        PWGM_ECNT_Tri(ecs) += PWGM_ECNT_Tri(tmp);
        PWGM_ECNT_Quad(ecs) += PWGM_ECNT_Quad(tmp);
        ++patch;
    }
    // write patch elements
    if (progressBeginStep(PWGM_ECNT_Tri(ecs) + PWGM_ECNT_Quad(ecs))) {
        Indent dent;
        unsigned long triCnt = PWGM_ECNT_Tri(ecs) + PWGM_ECNT_Quad(ecs) * 2;
        dent.comment("triangle count: %lu", triCnt);
        dent.puts("<volume>\n");
        dent.metadata(1, "Name", "Volume_%05lu", (unsigned long)0);
        patch.moveFirst(model_);
        while (patch.isValid()) {
            CaeUnsElement elem(patch);
            PWGM_ELEMDATA eData;
            while (elem.isValid() && elem.data(eData) && progressIncrement()) {
                writeElement(eData);
                ++elem;
            }
            ++patch;
        }
        dent.puts("</volume>\n");
    }
}


void
CaeUnsAMF::writeElement(const PWGM_ELEMDATA &eData) const
{
    if (3 > eData.vertCnt) {
        return;
    }
    writeTriangle(eData.index[0], eData.index[1], eData.index[2]);
    if (4 == eData.vertCnt) {
        writeTriangle(eData.index[0], eData.index[2], eData.index[3]);
    }
}


void
CaeUnsAMF::writeTriangle(PWP_UINT32 i0, PWP_UINT32 i1, PWP_UINT32 i2) const
{
    Indent dent;
    dent.puts("<triangle>\n");
    dent.printf(1, "<v1>%lu</v1>\n", (unsigned long)i0);
    dent.printf(1, "<v2>%lu</v2>\n", (unsigned long)i1);
    dent.printf(1, "<v3>%lu</v3>\n", (unsigned long)i2);
    dent.puts("</triangle>\n");
}


bool
CaeUnsAMF::endExport()
{
    return true;
}


const char *
CaeUnsAMF::unitName(PWP_UINT units)
{
    #define ENUMCASE(id) case id: ret = #id; break
    const char *ret = "error";
    switch (units) {
    ENUMCASE(mm);
    ENUMCASE(inch);
    ENUMCASE(ft);
    ENUMCASE(meters);
    ENUMCASE(micrometers);
    }
    return ret;
    #undef ENUMCASE
}



//===========================================================================
// face streaming handlers
//===========================================================================

PWP_UINT32
CaeUnsAMF::streamBegin(const PWGM_BEGINSTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "STREAM BEGIN: %lu", (unsigned long)data.totalNumFaces);
    sendInfoMsg(msg);
    return 1;
}

PWP_UINT32
CaeUnsAMF::streamFace(const PWGM_FACESTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "  STREAM FACE: %lu %lu", (unsigned long)data.owner.cellIndex,
        (unsigned long)data.face);
    sendInfoMsg(msg);
    return 1;
}

PWP_UINT32
CaeUnsAMF::streamEnd(const PWGM_ENDSTREAM_DATA &data)
{
    char msg[128];
    sprintf(msg, "STREAM END: %s", (data.ok ? "true" : "false"));
    sendInfoMsg(msg);
    return 1;
}


//===========================================================================
// called ONCE when plugin first loaded into memeory
//===========================================================================

bool
CaeUnsAMF::create(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
    bool ret = true;

    // Uncomment this INFO attribute if your solver supports both big and
    // little endian byte orderings.
    //ret = ret && caeuAssignInfoValue("AllowedFileByteOrders",
    //                "BigEndian|LittleEndian", true);

    // Uncomment one of these INFO attributes if your solver requires a
    // particular byte ordering.
    //ret = ret && caeuAssignInfoValue("AllowedFileByteOrders", "BigEndian",
    //                true);
    //ret = ret && caeuAssignInfoValue("AllowedFileByteOrders", "LittleEndian",
    //                true);

    // These attributes are for example only. You can publish any attribute
    // needed for your solver.
    // ret = ret &&
    //      caeuPublishValueDefinition("iterations", PWP_VALTYPE_UINT, "5",
    //          "RW", "Number of iterations", "0 2000") &&
    //      caeuPublishValueDefinition("magnitude", PWP_VALTYPE_INT, "-5",
    //          "RW", "Signed int magnitude", "-100 100") &&
    //      caeuPublishValueDefinition("mach", PWP_VALTYPE_REAL, "0.3", "RW",
    //          "Incoming flow velocity", "-Inf +Inf 0.0 50.0") &&
    //      caeuPublishValueDefinition("temperature", PWP_VALTYPE_REAL, "77.5",
    //          "RW", "Ambient temperature", "-Inf +Inf -100.0 3000.0") &&
    //      caeuPublishValueDefinition("temperature.units", PWP_VALTYPE_ENUM,
    //          "Fahrenheit", "RW", "Grid temperature units", TempUnitEnum) &&
    //      caeuPublishValueDefinition("description", PWP_VALTYPE_STRING, "",
    //          "RW", "Grid description", "") &&
    //      caeuPublishValueDefinition("linear", PWP_VALTYPE_BOOL, "reject",
    //          "RW", "Grid is linear", "reject|accept");

    ret = ret && caeuPublishValueDefinition(AttrUnits, PWP_VALTYPE_ENUM,
        unitName(CaeUnsAMF::inch), "RW", "Dimensional units",
        "mm|inch|ft|meters|micrometers");

    return ret;
}


//===========================================================================
// called ONCE just before plugin unloaded from memeory
//===========================================================================

void
CaeUnsAMF::destroy(CAEP_RTITEM &rti)
{
    (void)rti.BCCnt; // silence unused arg warning
}
