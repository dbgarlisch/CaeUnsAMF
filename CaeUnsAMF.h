/****************************************************************************
 *
 * class CaeUnsAMF
 *
 * Proprietary software product of Pointwise, Inc.
 * Copyright (c) 1995-2011 Pointwise, Inc.
 * All rights reserved.
 *
 ***************************************************************************/

#ifndef _CAEUNSAMF_H_
#define _CAEUNSAMF_H_

#include "apiGridModel.h"
#include "apiPWP.h"

#include "CaePlugin.h"
#include "CaeUnsGridModel.h"


//***************************************************************************
//***************************************************************************
//***************************************************************************

class CaeUnsAMF :
    public CaeUnsPlugin,
    public CaeFaceStreamHandler {

public:
    CaeUnsAMF(CAEP_RTITEM *pRti, PWGM_HGRIDMODEL model,
        const CAEP_WRITEINFO *pWriteInfo);
    ~CaeUnsAMF();
    static bool create(CAEP_RTITEM &rti);
    static void destroy(CAEP_RTITEM &rti);

private:
    enum UnitId {
        mm,
        inch,
        ft,
        meters,
        micrometers
    };
    static const char * unitName(PWP_UINT units);
    const char *        unitName() const {
                            return unitName(units_);
                        }

    void    writeObject(PWP_UINT id) const;
    void    writeMesh() const;
    void    writeVertices() const;
    void    writeVertex(const CaeUnsVertex &v) const;
    void    writeCoordinates(const CaeUnsVertex &v) const;

    void    writeVolume() const;
    void    writeVolume2() const;
    void    writeVolume3() const;
    void    writeElement(const PWGM_ELEMDATA &d) const;
    void    writeTriangle(PWP_UINT32 i0, PWP_UINT32 i1, PWP_UINT32 i2) const;

    virtual bool        beginExport();
    virtual PWP_BOOL    write();
    virtual bool        endExport();

    // face streaming handlers
    virtual PWP_UINT32 streamBegin(const PWGM_BEGINSTREAM_DATA &data);
    virtual PWP_UINT32 streamFace(const PWGM_FACESTREAM_DATA &data);
    virtual PWP_UINT32 streamEnd(const PWGM_ENDSTREAM_DATA &data);

private:
    PWP_UINT    units_;
};

#endif // _CAEUNSAMF_H_
