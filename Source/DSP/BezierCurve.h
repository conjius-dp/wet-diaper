#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

struct BezierHandle
{
    float dx = 0.0f;
    float dy = 0.0f;
};

struct BezierPoint
{
    float x;
    float y;
    BezierHandle in;
    BezierHandle out;
};

class BezierCurve
{
public:
    static constexpr int kLutSize = 2048;
    static constexpr int kMaxPoints = 5;
    static constexpr float kMinGap = 0.005f;

    BezierCurve();

    int addPoint(float x, float y);
    void removePoint(int index);
    void movePoint(int index, float newX, float newY);
    void moveInHandle(int index, float dx, float dy);
    void moveOutHandle(int index, float dx, float dy);
    void moveStartOutHandle(float dx, float dy);
    void moveEndInHandle(float dx, float dy);

    int getNumPoints() const { return static_cast<int>(points_.size()); }
    const BezierPoint& getPoint(int index) const { return points_[static_cast<size_t>(index)]; }
    BezierHandle getStartOutHandle() const { return startOut_; }
    BezierHandle getEndInHandle() const { return endIn_; }

    float evaluate(float x) const;
    void generateLUT(float* buffer) const;

    static float lookupWithGain(float sample, float gain, const float* lut, int lutSize);

    struct SlotValues
    {
        static constexpr int kMaxSlots = 5;

        float startOutDx = 1.0f / 3.0f;
        float startOutDy = 1.0f / 3.0f;
        float endInDx = -1.0f / 3.0f;
        float endInDy = -1.0f / 3.0f;

        struct Slot
        {
            bool on = false;
            float x = 0.5f;
            float y = 0.5f;
            float inDx = 0.0f;
            float inDy = 0.0f;
            float outDx = 0.0f;
            float outDy = 0.0f;
        };

        Slot slots[kMaxSlots]{};

        int curvePointToSlot(int curveIndex) const;
        int slotToCurvePoint(int slotIndex) const;
    };

    static void buildFromSlots(BezierCurve& curve, const SlotValues& vals);

    const std::vector<BezierPoint>& getPoints() const { return points_; }
    void setStartOutHandle(BezierHandle h) { startOut_ = h; }
    void setEndInHandle(BezierHandle h) { endIn_ = h; }
    void setPoints(std::vector<BezierPoint> pts) { points_ = std::move(pts); }
    void reset();

private:
    std::vector<BezierPoint> points_;
    BezierHandle startOut_{1.0f / 3.0f, 1.0f / 3.0f};
    BezierHandle endIn_{-1.0f / 3.0f, -1.0f / 3.0f};

    struct Segment
    {
        float x0, y0, x1, y1, x2, y2, x3, y3;
    };

    std::vector<Segment> getSegments() const;
    void clampHandlesForSegment(float p0x, float p3x, float& outDx, float& inDx);

    static float cubicBezier(float t, float p0, float p1, float p2, float p3);
    static float cubicBezierDeriv(float t, float p0, float p1, float p2, float p3);
    static float findT(float targetX, float x0, float x1, float x2, float x3);
    static float evalSegmentY(const Segment& seg, float targetX);
};
