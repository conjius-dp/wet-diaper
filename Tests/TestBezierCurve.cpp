#include <juce_core/juce_core.h>
#include "DSP/BezierCurve.h"
#include <cmath>
#include <vector>

class BezierCurveTests : public juce::UnitTest
{
public:
    BezierCurveTests() : juce::UnitTest("BezierCurve") {}

    void runTest() override
    {
        beginTest("default state is identity line");
        {
            BezierCurve curve;
            expectEquals(curve.getNumPoints(), 0);
            expectWithinAbsoluteError(curve.evaluate(0.0f), 0.0f, 0.001f);
            expectWithinAbsoluteError(curve.evaluate(0.5f), 0.5f, 0.001f);
            expectWithinAbsoluteError(curve.evaluate(1.0f), 1.0f, 0.001f);
            expectWithinAbsoluteError(curve.evaluate(0.25f), 0.25f, 0.001f);
            expectWithinAbsoluteError(curve.evaluate(0.75f), 0.75f, 0.001f);
        }

        beginTest("add point increases count");
        {
            BezierCurve curve;
            int idx = curve.addPoint(0.5f, 0.8f);
            expect(idx >= 0);
            expectEquals(curve.getNumPoints(), 1);
            auto& pt = curve.getPoint(0);
            expectWithinAbsoluteError(pt.x, 0.5f, 0.001f);
            expectWithinAbsoluteError(pt.y, 0.8f, 0.001f);
        }

        beginTest("add multiple points sorted by x");
        {
            BezierCurve curve;
            curve.addPoint(0.7f, 0.9f);
            curve.addPoint(0.3f, 0.5f);
            curve.addPoint(0.5f, 0.8f);
            expectEquals(curve.getNumPoints(), 3);
            expect(curve.getPoint(0).x < curve.getPoint(1).x);
            expect(curve.getPoint(1).x < curve.getPoint(2).x);
        }

        beginTest("cannot add point at x=0 or x=1");
        {
            BezierCurve curve;
            int idx0 = curve.addPoint(0.0f, 0.5f);
            int idx1 = curve.addPoint(1.0f, 0.5f);
            expectEquals(idx0, -1);
            expectEquals(idx1, -1);
            expectEquals(curve.getNumPoints(), 0);
        }

        beginTest("max 5 points enforced");
        {
            BezierCurve curve;
            for (int i = 1; i <= 5; ++i)
                curve.addPoint(static_cast<float>(i) / 6.0f, 0.5f);
            expectEquals(curve.getNumPoints(), 5);
            int overflow = curve.addPoint(0.95f, 0.5f);
            expectEquals(overflow, -1);
            expectEquals(curve.getNumPoints(), 5);
        }

        beginTest("remove point decreases count");
        {
            BezierCurve curve;
            curve.addPoint(0.3f, 0.5f);
            curve.addPoint(0.7f, 0.9f);
            expectEquals(curve.getNumPoints(), 2);
            curve.removePoint(0);
            expectEquals(curve.getNumPoints(), 1);
            expectWithinAbsoluteError(curve.getPoint(0).x, 0.7f, 0.001f);
        }

        beginTest("move point x clamped between neighbors");
        {
            BezierCurve curve;
            curve.addPoint(0.3f, 0.5f);
            curve.addPoint(0.7f, 0.9f);
            curve.movePoint(0, 0.8f, 0.5f);
            expect(curve.getPoint(0).x < curve.getPoint(1).x,
                   "moved point should stay before next point");
        }

        beginTest("move point y clamped to [-1, 1]");
        {
            BezierCurve curve;
            curve.addPoint(0.5f, 0.5f);
            curve.movePoint(0, 0.5f, 2.0f);
            expect(curve.getPoint(0).y <= 1.0f);
            curve.movePoint(0, 0.5f, -2.0f);
            expect(curve.getPoint(0).y >= -1.0f);
        }

        beginTest("outgoing handle dx clamped >= 0");
        {
            BezierCurve curve;
            curve.addPoint(0.5f, 0.5f);
            curve.moveOutHandle(0, -0.3f, 0.0f);
            expect(curve.getPoint(0).out.dx >= 0.0f);
        }

        beginTest("incoming handle dx clamped <= 0");
        {
            BezierCurve curve;
            curve.addPoint(0.5f, 0.5f);
            curve.moveInHandle(0, 0.3f, 0.0f);
            expect(curve.getPoint(0).in.dx <= 0.0f);
        }

        beginTest("handle does not cross segment boundary");
        {
            BezierCurve curve;
            curve.addPoint(0.5f, 0.5f);
            curve.moveOutHandle(0, 0.9f, 0.0f);
            float absX = curve.getPoint(0).x + curve.getPoint(0).out.dx;
            expect(absX <= 1.0f + 0.001f,
                   "outgoing handle should not exceed end point");
        }

        beginTest("LUT generation identity");
        {
            BezierCurve curve;
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            expectWithinAbsoluteError(lut[0], 0.0f, 0.001f);
            expectWithinAbsoluteError(lut[BezierCurve::kLutSize - 1], 1.0f, 0.001f);

            int mid = BezierCurve::kLutSize / 2;
            float expectedMid = static_cast<float>(mid) / static_cast<float>(BezierCurve::kLutSize - 1);
            expectWithinAbsoluteError(lut[mid], expectedMid, 0.002f);

            for (int i = 1; i < BezierCurve::kLutSize; ++i)
                expect(lut[i] >= lut[i - 1] - 0.001f,
                       "default LUT should be monotonically non-decreasing");
        }

        beginTest("LUT generation with user point");
        {
            BezierCurve curve;
            curve.addPoint(0.3f, 0.9f);
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            int idx30 = static_cast<int>(0.3f * (BezierCurve::kLutSize - 1));
            expect(lut[idx30] > 0.5f,
                   "LUT at x=0.3 should reflect the high y-value of the user point");

            expectWithinAbsoluteError(lut[0], 0.0f, 0.001f);
            expectWithinAbsoluteError(lut[BezierCurve::kLutSize - 1], 1.0f, 0.001f);
        }

        beginTest("lookupWithGain identity");
        {
            BezierCurve curve;
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            float result = BezierCurve::lookupWithGain(0.5f, 1.0f, lut, BezierCurve::kLutSize);
            expectWithinAbsoluteError(result, 0.5f, 0.01f);
        }

        beginTest("lookupWithGain symmetry");
        {
            BezierCurve curve;
            curve.addPoint(0.4f, 0.85f);
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            float pos = BezierCurve::lookupWithGain(0.3f, 1.0f, lut, BezierCurve::kLutSize);
            float neg = BezierCurve::lookupWithGain(-0.3f, 1.0f, lut, BezierCurve::kLutSize);
            expectWithinAbsoluteError(pos, -neg, 0.001f);
        }

        beginTest("lookupWithGain clamping with high gain");
        {
            BezierCurve curve;
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            float result = BezierCurve::lookupWithGain(0.5f, 100.0f, lut, BezierCurve::kLutSize);
            expectWithinAbsoluteError(result, 1.0f, 0.01f);
        }

        beginTest("lookupWithGain zero input");
        {
            BezierCurve curve;
            float lut[BezierCurve::kLutSize];
            curve.generateLUT(lut);

            float result = BezierCurve::lookupWithGain(0.0f, 50.0f, lut, BezierCurve::kLutSize);
            expectWithinAbsoluteError(result, 0.0f, 0.001f);
        }

        beginTest("reset restores default identity");
        {
            BezierCurve curve;
            curve.addPoint(0.5f, 0.8f);
            curve.moveStartOutHandle(0.1f, 0.9f);
            curve.reset();
            expectEquals(curve.getNumPoints(), 0);
            expectWithinAbsoluteError(curve.evaluate(0.5f), 0.5f, 0.001f);
            auto h = curve.getStartOutHandle();
            expectWithinAbsoluteError(h.dx, 1.0f / 3.0f, 0.001f);
        }

        beginTest("setPoints and getPoints round-trip");
        {
            BezierCurve original;
            original.addPoint(0.3f, 0.7f);
            original.addPoint(0.6f, 0.95f);

            auto pts = original.getPoints();
            BezierCurve copy;
            copy.setPoints(pts);
            copy.setStartOutHandle(original.getStartOutHandle());
            copy.setEndInHandle(original.getEndInHandle());

            expectEquals(copy.getNumPoints(), 2);
            expectWithinAbsoluteError(copy.getPoint(0).x, 0.3f, 0.001f);
            expectWithinAbsoluteError(copy.getPoint(1).y, 0.95f, 0.001f);
        }

        beginTest("start handle out dx clamped correctly");
        {
            BezierCurve curve;
            curve.moveStartOutHandle(-0.5f, 0.0f);
            auto h = curve.getStartOutHandle();
            expect(h.dx >= 0.0f, "start out handle dx should be >= 0");
        }

        beginTest("end handle in dx clamped correctly");
        {
            BezierCurve curve;
            curve.moveEndInHandle(0.5f, 0.0f);
            auto h = curve.getEndInHandle();
            expect(h.dx <= 0.0f, "end in handle dx should be <= 0");
        }

        beginTest("buildFromSlots default produces identity");
        {
            BezierCurve::SlotValues vals;
            BezierCurve curve;
            BezierCurve::buildFromSlots(curve, vals);
            expectEquals(curve.getNumPoints(), 0);
            expectWithinAbsoluteError(curve.evaluate(0.5f), 0.5f, 0.001f);
            auto sh = curve.getStartOutHandle();
            expectWithinAbsoluteError(sh.dx, 1.0f / 3.0f, 0.001f);
            expectWithinAbsoluteError(sh.dy, 1.0f / 3.0f, 0.001f);
        }

        beginTest("buildFromSlots one active slot");
        {
            BezierCurve::SlotValues vals;
            vals.slots[2].on = true;
            vals.slots[2].x = 0.4f;
            vals.slots[2].y = 0.9f;
            BezierCurve curve;
            BezierCurve::buildFromSlots(curve, vals);
            expectEquals(curve.getNumPoints(), 1);
            expectWithinAbsoluteError(curve.getPoint(0).x, 0.4f, 0.001f);
            expectWithinAbsoluteError(curve.getPoint(0).y, 0.9f, 0.001f);
        }

        beginTest("buildFromSlots multiple slots sorted by x");
        {
            BezierCurve::SlotValues vals;
            vals.slots[3].on = true;
            vals.slots[3].x = 0.7f;
            vals.slots[3].y = 0.8f;
            vals.slots[0].on = true;
            vals.slots[0].x = 0.3f;
            vals.slots[0].y = 0.5f;
            BezierCurve curve;
            BezierCurve::buildFromSlots(curve, vals);
            expectEquals(curve.getNumPoints(), 2);
            expect(curve.getPoint(0).x < curve.getPoint(1).x);
            expectWithinAbsoluteError(curve.getPoint(0).x, 0.3f, 0.001f);
            expectWithinAbsoluteError(curve.getPoint(1).x, 0.7f, 0.001f);
        }

        beginTest("buildFromSlots inactive slots skipped");
        {
            BezierCurve::SlotValues vals;
            vals.slots[0].on = true;
            vals.slots[0].x = 0.5f;
            vals.slots[0].y = 0.8f;
            vals.slots[1].on = false;
            vals.slots[1].x = 0.3f;
            vals.slots[1].y = 0.6f;
            BezierCurve curve;
            BezierCurve::buildFromSlots(curve, vals);
            expectEquals(curve.getNumPoints(), 1);
        }

        beginTest("buildFromSlots preserves handles");
        {
            BezierCurve::SlotValues vals;
            vals.startOutDx = 0.2f;
            vals.startOutDy = 0.5f;
            vals.endInDx = -0.2f;
            vals.endInDy = -0.3f;
            vals.slots[0].on = true;
            vals.slots[0].x = 0.5f;
            vals.slots[0].y = 0.7f;
            vals.slots[0].inDx = -0.1f;
            vals.slots[0].inDy = 0.05f;
            vals.slots[0].outDx = 0.1f;
            vals.slots[0].outDy = -0.05f;
            BezierCurve curve;
            BezierCurve::buildFromSlots(curve, vals);
            auto sh = curve.getStartOutHandle();
            expectWithinAbsoluteError(sh.dx, 0.2f, 0.001f);
            expectWithinAbsoluteError(sh.dy, 0.5f, 0.001f);
            auto eh = curve.getEndInHandle();
            expectWithinAbsoluteError(eh.dx, -0.2f, 0.001f);
            expectWithinAbsoluteError(eh.dy, -0.3f, 0.001f);
            auto& pt = curve.getPoint(0);
            expectWithinAbsoluteError(pt.in.dx, -0.1f, 0.001f);
            expectWithinAbsoluteError(pt.in.dy, 0.05f, 0.001f);
            expectWithinAbsoluteError(pt.out.dx, 0.1f, 0.001f);
            expectWithinAbsoluteError(pt.out.dy, -0.05f, 0.001f);
        }

        beginTest("curvePointToSlot mapping");
        {
            BezierCurve::SlotValues vals;
            vals.slots[3].on = true;
            vals.slots[3].x = 0.7f;
            vals.slots[3].y = 0.8f;
            vals.slots[0].on = true;
            vals.slots[0].x = 0.3f;
            vals.slots[0].y = 0.5f;
            expectEquals(vals.curvePointToSlot(0), 0);
            expectEquals(vals.curvePointToSlot(1), 3);
            expectEquals(vals.curvePointToSlot(2), -1);
        }

        beginTest("slotToCurvePoint mapping");
        {
            BezierCurve::SlotValues vals;
            vals.slots[3].on = true;
            vals.slots[3].x = 0.7f;
            vals.slots[0].on = true;
            vals.slots[0].x = 0.3f;
            expectEquals(vals.slotToCurvePoint(0), 0);
            expectEquals(vals.slotToCurvePoint(3), 1);
            expectEquals(vals.slotToCurvePoint(1), -1);
        }
    }
};

static BezierCurveTests bezierCurveTests;
