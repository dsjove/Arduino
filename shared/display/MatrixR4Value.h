#pragma once

#include <array>
#include <cstdint>
#include <algorithm>
#include <limits>

class MatrixR4Value
{
public:
  static constexpr int Width      = 12;
  static constexpr int Height     = 8;
  static constexpr int WordCount  = 3;
  static constexpr int TotalBits  = Width * Height;

  using Value = std::array<uint32_t, WordCount>;
  using Frames = std::array<MatrixR4Value, 11>;

  static constexpr Value allOff = { 0x00000000u, 0x00000000u, 0x00000000u };
  static constexpr Value allOn  = { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu };
  static constexpr Value circle = { 0x06009010u, 0x82042041u, 0x08090060u };

  MatrixR4Value(bool inverting = false)
  : _value(inverting ? allOn : allOff)
  {
  }

  explicit MatrixR4Value(const Value& v)
  : _value(v)
  {
  }

  inline const uint32_t* data() const { return _value.data(); }
  inline size_t size() const { return sizeof(_value); }

  bool update(const Value& input, bool flippingY = false, bool flippingX = false, bool inverting = false)
  {
    Value newValue;
    if (!flippingY && !flippingX)
    {
      newValue = inverting ? Value{ ~input[0], ~input[1], ~input[2] } : input;
    }
    else
    {
      for (int y = 0; y < Height; ++y)
      {
        for (int x = 0; x < Width; ++x)
        {
          const int srcY = flippingY ? flipY(y) : y;
          const int srcX = flippingX ? flipX(x) : x;
          const int srcIndex = getIndex(srcY, srcX);
          const int dstIndex = getIndex(y, x);
          bool pixel = getBit(input, srcIndex);
          if (inverting) pixel = !pixel;
          setBit(newValue, dstIndex, pixel);
        }
      }
    }
    if (_value == newValue) return false;
    _value = newValue;
    return true;
  }

  void animateTo(const MatrixR4Value& last, Frames& frames, int& count) const
  {
    count = 0;

    const Value src = _value;
    const Value dst = last._value;

    if (src == dst)
    {
      frames[0] = last;
      count = 1;
      return;
    }

    Value overlap = allOff;
    Value srcOnly = allOff;
    Value dstOnly = allOff;
    for (int i=0;i<WordCount;++i)
    {
      overlap[i] = src[i] & dst[i];
      srcOnly[i] = src[i] & ~dst[i];
      dstOnly[i] = dst[i] & ~src[i];
    }

    struct Pt { int x; int y; };
    struct Rect { int l; int t; int r; int b; };
    struct Mover { Pt cur; Pt dst; };

    auto rectEmpty = [](const Rect& rc){ return rc.r < rc.l || rc.b < rc.t; };
    auto sign = [](int v){ return (v>0)-(v<0); };

    auto chebyshev = [](Pt a, Pt b){
      int dx = (a.x>b.x)?a.x-b.x:b.x-a.x;
      int dy = (a.y>b.y)?a.y-b.y:b.y-a.y;
      return dx>dy?dx:dy;
    };

    auto orValue = [&](Value& a,const Value& b){
      for(int i=0;i<WordCount;++i) a[i]|=b[i];
    };

    auto setPixel = [&](Value& v,int x,int y,bool on){
      setBit(v,getIndex(y,x),on);
    };

    auto collectPts = [&](const Value& v, std::array<Pt,TotalBits>& out, int& n){
      n=0;
      for(int y=0;y<Height;++y)
        for(int x=0;x<Width;++x)
          if(getBit(v,getIndex(y,x)))
            out[n++] = Pt{x,y};
    };

    auto bboxOf = [&](const std::array<Pt,TotalBits>& pts,int n)->Rect{
      Rect rc{Width,Height,-1,-1};
      for(int i=0;i<n;++i){
        rc.l=std::min(rc.l,pts[i].x);
        rc.t=std::min(rc.t,pts[i].y);
        rc.r=std::max(rc.r,pts[i].x);
        rc.b=std::max(rc.b,pts[i].y);
      }
      if(rc.r<rc.l||rc.b<rc.t) return Rect{0,0,-1,-1};
      return rc;
    };

    auto lerpInt = [](int a,int b,int num,int den){
      return (int)(( (int64_t)a*(den-num)+(int64_t)b*num + den/2)/den);
    };

    auto shrinkRect = [&](Rect s,int step,int steps)->Rect{
      if(rectEmpty(s)) return s;
      int mx=(s.l+s.r)/2, my=(s.t+s.b)/2;
      Rect e{mx+1,my+1,mx,my};
      return Rect{
        lerpInt(s.l,e.l,step,steps),
        lerpInt(s.t,e.t,step,steps),
        lerpInt(s.r,e.r,step,steps),
        lerpInt(s.b,e.b,step,steps)
      };
    };

    auto growRect = [&](Rect s,Rect e,int step,int steps)->Rect{
      Rect r{
        lerpInt(s.l,e.l,step,steps),
        lerpInt(s.t,e.t,step,steps),
        lerpInt(s.r,e.r,step,steps),
        lerpInt(s.b,e.b,step,steps)
      };
      r.l=std::max(0,std::min(Width-1,r.l));
      r.r=std::max(0,std::min(Width-1,r.r));
      r.t=std::max(0,std::min(Height-1,r.t));
      r.b=std::max(0,std::min(Height-1,r.b));
      return r;
    };

    auto push = [&](const Value& v){
      if(count < (int)frames.size())
        frames[count++] = MatrixR4Value(v);
    };

    auto isAllOff = [&](const Value& v){
      return v[0]==0 && v[1]==0 && v[2]==0;
    };

    // -------- allOff -> non-allOff reveal --------
    if(isAllOff(src) && !isAllOff(dst))
    {
      std::array<Pt,TotalBits> pts{};
      int n=0; collectPts(dst,pts,n);
      Rect box=bboxOf(pts,n);
      if(rectEmpty(box)){ push(dst); return; }

      int cx=(box.l+box.r)/2, cy=(box.t+box.b)/2;
      Rect start{cx,cy,cx,cy};
      int steps = std::min(10,std::max(1,std::max(box.r-box.l,box.b-box.t)));

      for(int s=1;s<=steps && count<10;++s)
      {
        Rect rc=growRect(start,box,s,steps);
        Value f=allOff;
        for(int i=0;i<n;++i)
          if(pts[i].x>=rc.l && pts[i].x<=rc.r && pts[i].y>=rc.t && pts[i].y<=rc.b)
            setPixel(f,pts[i].x,pts[i].y,true);
        push(f);
      }
      push(dst);
      return;
    }

    // -------- general case --------
    std::array<Pt,TotalBits> srcOnlyPts{}, dstOnlyPts{}, srcOnPts{};
    int srcOnlyN=0,dstOnlyN=0,srcOnN=0;
    collectPts(srcOnly,srcOnlyPts,srcOnlyN);
    collectPts(dstOnly,dstOnlyPts,dstOnlyN);
    collectPts(src,srcOnPts,srcOnN);

    Rect srcBox=bboxOf(srcOnlyPts,srcOnlyN);

    std::array<Mover,TotalBits> movers{};
    int moverN=dstOnlyN;
    int maxMove=0;

    for(int i=0;i<dstOnlyN;++i)
    {
      Pt d=dstOnlyPts[i];
      Pt best=d;
      int bd=std::numeric_limits<int>::max();
      for(int j=0;j<srcOnN;++j)
      {
        int dist=chebyshev(srcOnPts[j],d);
        if(dist<bd){bd=dist;best=srcOnPts[j];}
      }
      movers[i]={best,d};
      if(bd!=std::numeric_limits<int>::max())
        maxMove=std::max(maxMove,bd);
    }

    int disappear = rectEmpty(srcBox)?0:std::max(srcBox.r-srcBox.l,srcBox.b-srcBox.t)+1;
    int steps = std::min(10,std::max({1,maxMove,disappear}));

    for(int s=1;s<=steps && count<10;++s)
    {
      Value f=allOff;
      orValue(f,overlap);

      if(srcOnlyN>0)
      {
        Rect rc=shrinkRect(srcBox,s,steps);
        if(!rectEmpty(rc))
          for(int i=0;i<srcOnlyN;++i)
            if(srcOnlyPts[i].x>=rc.l && srcOnlyPts[i].x<=rc.r &&
               srcOnlyPts[i].y>=rc.t && srcOnlyPts[i].y<=rc.b)
              setPixel(f,srcOnlyPts[i].x,srcOnlyPts[i].y,true);
      }

      for(int i=0;i<moverN;++i)
      {
        movers[i].cur.x += sign(movers[i].dst.x - movers[i].cur.x);
        movers[i].cur.y += sign(movers[i].dst.y - movers[i].cur.y);
        setPixel(f,movers[i].cur.x,movers[i].cur.y,true);
      }

      push(f);
    }

    push(dst);
  }

private:
  Value _value;

  inline static int getIndex(int y, int x)
  {
    return y * Width + x;
  }

  inline static int flipY(int y)
  {
    return Height - 1 - y;
  }

  inline static int flipX(int x)
  {
    return Width - 1 - x;
  }

  inline static bool getBit(const Value& frame, int index)
  {
    const uint32_t word = frame[index / 32];
    const uint32_t mask = 1u << (31 - (index % 32));  // MSB-first
    return (word & mask) != 0;
  }

  inline static void setBit(Value& frame, int index, bool value)
  {
    uint32_t& word = frame[index / 32];
    const uint32_t mask = 1u << (31 - (index % 32));  // MSB-first
    word = value ? (word | mask) : (word & ~mask);
  }
};
