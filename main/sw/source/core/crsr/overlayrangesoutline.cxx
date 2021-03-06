/**************************************************************
* 
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
* 
*************************************************************/


// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sw.hxx"

#include <overlayrangesoutline.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygoncutter.hxx>
#include <drawinglayer/primitive2d/polypolygonprimitive2d.hxx>

//////////////////////////////////////////////////////////////////////////////

namespace
{
    // combine ranges geometrically to a single, ORed polygon
    basegfx::B2DPolyPolygon impCombineRangesToPolyPolygon(const std::vector< basegfx::B2DRange >& rRanges)
    {
        const sal_uInt32 nCount(rRanges.size());
        basegfx::B2DPolyPolygon aRetval;

        for(sal_uInt32 a(0); a < nCount; a++)
        {
            const basegfx::B2DPolygon aDiscretePolygon(basegfx::tools::createPolygonFromRect(rRanges[a]));

            if(0 == a)
            {
                aRetval.append(aDiscretePolygon);
            }
            else
            {
                aRetval = basegfx::tools::solvePolygonOperationOr(aRetval, basegfx::B2DPolyPolygon(aDiscretePolygon));
            }
        }

        return aRetval;
    }
}

namespace sw
{
    namespace overlay
    {
        drawinglayer::primitive2d::Primitive2DSequence OverlayRangesOutline::createOverlayObjectPrimitive2DSequence()
        {
            drawinglayer::primitive2d::Primitive2DSequence aRetval;
            const sal_uInt32 nCount(getRanges().size());

            if( nCount )
            {
                const basegfx::BColor aRGBColor(getBaseColor().getBColor());
                const basegfx::B2DPolyPolygon aPolyPolygon(impCombineRangesToPolyPolygon(getRanges()));
                const drawinglayer::primitive2d::Primitive2DReference aOutline(
                    new drawinglayer::primitive2d::PolyPolygonHairlinePrimitive2D(
                    aPolyPolygon,
                    aRGBColor));

                aRetval.realloc(1);
                aRetval[0] = aOutline;
            }

            return aRetval;
        }

        OverlayRangesOutline::OverlayRangesOutline(
            const Color& rColor,
            const std::vector< basegfx::B2DRange >& rRanges )
            : sdr::overlay::OverlayObject(rColor)
            , maRanges(rRanges)
        {
            // no AA for highlight overlays
            allowAntiAliase(false);
        }

        OverlayRangesOutline::~OverlayRangesOutline()
        {
            if( getOverlayManager() )
            {
                getOverlayManager()->remove(*this);
            }
        }

        void OverlayRangesOutline::setRanges(const std::vector< basegfx::B2DRange >& rNew)
        {
            if(rNew != maRanges)
            {
                maRanges = rNew;
                objectChange();
            }
        }
    } // end of namespace overlay
} // end of namespace sdr

//////////////////////////////////////////////////////////////////////////////
// eof
