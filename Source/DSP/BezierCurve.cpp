#include "DSP/BezierCurve.h"

BezierCurve::BezierCurve() = default;

void BezierCurve::reset()
{
    points_.clear();
    startOut_ = {1.0f / 3.0f, 1.0f / 3.0f};
    endIn_ = {-1.0f / 3.0f, -1.0f / 3.0f};
}

int BezierCurve::addPoint(float x, float y)
{
    if (x <= kMinGap || x >= 1.0f - kMinGap)
        return -1;
    if (static_cast<int>(points_.size()) >= kMaxPoints)
        return -1;

    y = std::clamp(y, -1.0f, 1.0f);

    float prevX = 0.0f;
    float nextX = 1.0f;
    int insertIdx = static_cast<int>(points_.size());

    for (int i = 0; i < static_cast<int>(points_.size()); ++i)
    {
        if (points_[static_cast<size_t>(i)].x > x)
        {
            insertIdx = i;
            nextX = points_[static_cast<size_t>(i)].x;
            break;
        }
        prevX = points_[static_cast<size_t>(i)].x;
    }

    if (insertIdx < static_cast<int>(points_.size()))
    {
    }
    else
    {
        if (!points_.empty())
            prevX = points_.back().x;
    }

    if (x <= prevX + kMinGap || x >= nextX - kMinGap)
        return -1;

    float distPrev = x - prevX;
    float distNext = nextX - x;
    float inDx = -distPrev / 3.0f;
    float outDx = distNext / 3.0f;

    BezierPoint pt;
    pt.x = x;
    pt.y = y;
    pt.in = {inDx, 0.0f};
    pt.out = {outDx, 0.0f};

    points_.insert(points_.begin() + insertIdx, pt);
    return insertIdx;
}

void BezierCurve::removePoint(int index)
{
    if (index >= 0 && index < static_cast<int>(points_.size()))
        points_.erase(points_.begin() + index);
}

void BezierCurve::movePoint(int index, float newX, float newY)
{
    if (index < 0 || index >= static_cast<int>(points_.size()))
        return;

    float prevX = (index > 0) ? points_[static_cast<size_t>(index - 1)].x : 0.0f;
    float nextX = (index < static_cast<int>(points_.size()) - 1)
                      ? points_[static_cast<size_t>(index + 1)].x
                      : 1.0f;

    newX = std::clamp(newX, prevX + kMinGap, nextX - kMinGap);
    newY = std::clamp(newY, -1.0f, 1.0f);

    auto& pt = points_[static_cast<size_t>(index)];
    pt.x = newX;
    pt.y = newY;

    float distPrev = pt.x - prevX;
    float distNext = nextX - pt.x;
    pt.in.dx = std::clamp(pt.in.dx, -distPrev, 0.0f);
    pt.out.dx = std::clamp(pt.out.dx, 0.0f, distNext);

    pt.in.dy = std::clamp(pt.in.dy, -1.0f - pt.y, 1.0f - pt.y);
    pt.out.dy = std::clamp(pt.out.dy, -1.0f - pt.y, 1.0f - pt.y);
}

void BezierCurve::moveInHandle(int index, float dx, float dy)
{
    if (index < 0 || index >= static_cast<int>(points_.size()))
        return;

    auto& pt = points_[static_cast<size_t>(index)];
    float prevX = (index > 0) ? points_[static_cast<size_t>(index - 1)].x : 0.0f;
    float distPrev = pt.x - prevX;

    pt.in.dx = std::clamp(dx, -distPrev, 0.0f);
    pt.in.dy = std::clamp(dy, -1.0f - pt.y, 1.0f - pt.y);
}

void BezierCurve::moveOutHandle(int index, float dx, float dy)
{
    if (index < 0 || index >= static_cast<int>(points_.size()))
        return;

    auto& pt = points_[static_cast<size_t>(index)];
    float nextX = (index < static_cast<int>(points_.size()) - 1)
                      ? points_[static_cast<size_t>(index + 1)].x
                      : 1.0f;
    float distNext = nextX - pt.x;

    pt.out.dx = std::clamp(dx, 0.0f, distNext);
    pt.out.dy = std::clamp(dy, -1.0f - pt.y, 1.0f - pt.y);
}

void BezierCurve::moveStartOutHandle(float dx, float dy)
{
    float nextX = points_.empty() ? 1.0f : points_.front().x;
    startOut_.dx = std::clamp(dx, 0.0f, nextX);
    startOut_.dy = std::clamp(dy, -1.0f, 1.0f);
}

void BezierCurve::moveEndInHandle(float dx, float dy)
{
    float prevX = points_.empty() ? 0.0f : points_.back().x;
    float distPrev = 1.0f - prevX;
    endIn_.dx = std::clamp(dx, -distPrev, 0.0f);
    endIn_.dy = std::clamp(dy, -1.0f - 1.0f, 1.0f - 1.0f);
}

float BezierCurve::cubicBezier(float t, float p0, float p1, float p2, float p3)
{
    float u = 1.0f - t;
    return u * u * u * p0 + 3.0f * u * u * t * p1 + 3.0f * u * t * t * p2 + t * t * t * p3;
}

float BezierCurve::cubicBezierDeriv(float t, float p0, float p1, float p2, float p3)
{
    float u = 1.0f - t;
    return 3.0f * (u * u * (p1 - p0) + 2.0f * u * t * (p2 - p1) + t * t * (p3 - p2));
}

float BezierCurve::findT(float targetX, float x0, float x1, float x2, float x3)
{
    if (std::abs(x3 - x0) < 1e-7f)
        return 0.5f;

    float t = (targetX - x0) / (x3 - x0);
    t = std::clamp(t, 0.0f, 1.0f);

    for (int i = 0; i < 8; ++i)
    {
        float xAtT = cubicBezier(t, x0, x1, x2, x3);
        float err = xAtT - targetX;
        if (std::abs(err) < 1e-6f)
            break;
        float deriv = cubicBezierDeriv(t, x0, x1, x2, x3);
        if (std::abs(deriv) < 1e-10f)
            break;
        t -= err / deriv;
        t = std::clamp(t, 0.0f, 1.0f);
    }
    return t;
}

float BezierCurve::evalSegmentY(const Segment& seg, float targetX)
{
    float t = findT(targetX, seg.x0, seg.x1, seg.x2, seg.x3);
    return cubicBezier(t, seg.y0, seg.y1, seg.y2, seg.y3);
}

std::vector<BezierCurve::Segment> BezierCurve::getSegments() const
{
    std::vector<Segment> segs;

    struct Knot
    {
        float x, y;
        float inDx, inDy;
        float outDx, outDy;
    };

    std::vector<Knot> knots;
    knots.push_back({0.0f, 0.0f, 0.0f, 0.0f, startOut_.dx, startOut_.dy});

    for (auto& pt : points_)
        knots.push_back({pt.x, pt.y, pt.in.dx, pt.in.dy, pt.out.dx, pt.out.dy});

    knots.push_back({1.0f, 1.0f, endIn_.dx, endIn_.dy, 0.0f, 0.0f});

    for (size_t i = 0; i + 1 < knots.size(); ++i)
    {
        auto& k0 = knots[i];
        auto& k1 = knots[i + 1];

        Segment seg;
        seg.x0 = k0.x;
        seg.y0 = k0.y;
        seg.x1 = k0.x + k0.outDx;
        seg.y1 = k0.y + k0.outDy;
        seg.x2 = k1.x + k1.inDx;
        seg.y2 = k1.y + k1.inDy;
        seg.x3 = k1.x;
        seg.y3 = k1.y;

        seg.x1 = std::clamp(seg.x1, seg.x0, seg.x3);
        seg.x2 = std::clamp(seg.x2, seg.x0, seg.x3);
        if (seg.x1 > seg.x2)
        {
            float mid = (seg.x1 + seg.x2) * 0.5f;
            seg.x1 = mid;
            seg.x2 = mid;
        }

        segs.push_back(seg);
    }

    return segs;
}

float BezierCurve::evaluate(float x) const
{
    x = std::clamp(x, 0.0f, 1.0f);

    auto segs = getSegments();
    if (segs.empty())
        return x;

    for (auto& seg : segs)
    {
        if (x <= seg.x3 || &seg == &segs.back())
            return evalSegmentY(seg, x);
    }

    return x;
}

void BezierCurve::generateLUT(float* buffer) const
{
    auto segs = getSegments();

    if (segs.empty())
    {
        for (int i = 0; i < kLutSize; ++i)
            buffer[i] = static_cast<float>(i) / static_cast<float>(kLutSize - 1);
        return;
    }

    int segIdx = 0;
    for (int i = 0; i < kLutSize; ++i)
    {
        float targetX = static_cast<float>(i) / static_cast<float>(kLutSize - 1);

        while (segIdx < static_cast<int>(segs.size()) - 1 && targetX > segs[static_cast<size_t>(segIdx)].x3)
            ++segIdx;

        buffer[i] = evalSegmentY(segs[static_cast<size_t>(segIdx)], targetX);
    }

    buffer[0] = 0.0f;
    buffer[kLutSize - 1] = segs.back().y3;
}

float BezierCurve::lookupWithGain(float sample, float gain, const float* lut, int lutSize)
{
    float x = sample * gain;
    float sign = 1.0f;
    if (x < 0.0f)
    {
        sign = -1.0f;
        x = -x;
    }
    if (x > 1.0f)
        x = 1.0f;

    float pos = x * static_cast<float>(lutSize - 1);
    int idx = static_cast<int>(pos);
    float frac = pos - static_cast<float>(idx);
    if (idx >= lutSize - 1)
        return sign * lut[lutSize - 1];

    return sign * (lut[idx] + frac * (lut[idx + 1] - lut[idx]));
}

