#include "scanner.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace ordered_pdf {
namespace {
struct Point { double x, y; };
using Quad = std::array<Point, 4>;
double Cross(Point a, Point b, Point c) {
    return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
}
double Distance(Point a, Point b) { return std::hypot(a.x-b.x,a.y-b.y); }
double Area(const std::vector<Point>& p) {
    double result=0;
    for (size_t i=0;i<p.size();++i) result+=p[i].x*p[(i+1)%p.size()].y-p[i].y*p[(i+1)%p.size()].x;
    return std::abs(result)*0.5;
}
std::vector<Point> Hull(std::vector<Point> p) {
    std::sort(p.begin(),p.end(),[](Point a,Point b){return a.x<b.x || (a.x==b.x && a.y<b.y);});
    std::vector<Point> h;
    for (auto q:p) {
        while(h.size()>1 && Cross(h[h.size()-2],h.back(),q)<=0) h.pop_back();
        h.push_back(q);
    }
    const size_t lower=h.size();
    for (auto it=p.rbegin()+1;it!=p.rend();++it) {
        while(h.size()>lower && Cross(h[h.size()-2],h.back(),*it)<=0) h.pop_back();
        h.push_back(*it);
    }
    if(!h.empty()) h.pop_back();
    return h;
}
struct Pixels {
    Gdiplus::Bitmap& bitmap;
    Gdiplus::BitmapData data{};
    Pixels(Gdiplus::Bitmap& b, UINT mode):bitmap(b) {
        Gdiplus::Rect rect(0,0,b.GetWidth(),b.GetHeight());
        if(b.LockBits(&rect,mode,PixelFormat24bppRGB,&data)!=Gdiplus::Ok)
            throw Error(L"无法读取扫描图片像素。");
    }
    ~Pixels(){bitmap.UnlockBits(&data);}
    BYTE* Row(int y){return static_cast<BYTE*>(data.Scan0)+static_cast<ptrdiff_t>(y)*data.Stride;}
};
int Gray(const BYTE* p) {return (29*p[0]+150*p[1]+77*p[2])>>8;}
Quad Detect(Gdiplus::Bitmap& source) {
    const double scale=std::min(1.0,800.0/std::max(source.GetWidth(),source.GetHeight()));
    const int w=std::max(1,static_cast<int>(source.GetWidth()*scale));
    const int h=std::max(1,static_cast<int>(source.GetHeight()*scale));
    if(w<40 || h<40) throw Error(L"图片太小，无法扫描。请使用清晰的纸张照片。");
    Gdiplus::Bitmap small(w,h,PixelFormat24bppRGB);
    { Gdiplus::Graphics g(&small); g.Clear(Gdiplus::Color::White);
      g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
      if(g.DrawImage(&source,0,0,w,h)!=Gdiplus::Ok) throw Error(L"扫描图片缩放失败。"); }
    std::vector<BYTE> gray(w*h);
    { Pixels pixels(small,Gdiplus::ImageLockModeRead);
      for(int y=0;y<h;++y) for(int x=0;x<w;++x) gray[y*w+x]=static_cast<BYTE>(Gray(pixels.Row(y)+3*x)); }
    double bestScore=0;
    Quad best{};
    // Multiple thresholds tolerate uneven illumination. Connected components
    // separate the sheet from unrelated bright objects on the desk.
    for(int threshold: {100,130,155,180,205,225}) {
        std::vector<BYTE> visited(w*h);
        std::vector<int> queue; queue.reserve(w*h);
        for(int start=0;start<w*h;++start) {
            if(visited[start] || gray[start]<threshold) continue;
            queue.clear(); queue.push_back(start); visited[start]=1;
            std::vector<Point> boundary;
            int border=0;
            for(size_t i=0;i<queue.size();++i) {
                const int pos=queue[i],x=pos%w,y=pos/w;
                if(x==0 || y==0 || x==w-1 || y==h-1) ++border;
                bool edge=false;
                for(int d=0;d<4;++d) {
                    const int nx=x+(d==0)-(d==1),ny=y+(d==2)-(d==3);
                    if(nx<0 || nx>=w || ny<0 || ny>=h) {edge=true;continue;}
                    const int next=ny*w+nx;
                    if(gray[next]<threshold) {edge=true;continue;}
                    if(!visited[next]) {visited[next]=1;queue.push_back(next);}
                }
                if(edge) boundary.push_back({static_cast<double>(x),static_cast<double>(y)});
            }
            if(queue.size()<static_cast<size_t>(w*h*0.15) || border>2 || boundary.size()<4) continue;
            auto hull=Hull(std::move(boundary));
            const double hullArea=Area(hull);
            if(hull.size()<4 || hullArea<1) continue;
            // Reduce the convex hull to its four dominant corners. Reject
            // rounded/irregular objects whose area is poorly explained by a quad.
            while(hull.size()>4) {
                size_t remove=0; double smallest=1e30;
                for(size_t i=0;i<hull.size();++i) {
                    const double loss=std::abs(Cross(hull[(i+hull.size()-1)%hull.size()],hull[i],hull[(i+1)%hull.size()]));
                    if(loss<smallest){smallest=loss;remove=i;}
                }
                hull.erase(hull.begin()+static_cast<ptrdiff_t>(remove));
            }
            const double area=Area(hull), fit=area/hullArea, fill=queue.size()/hullArea;
            if(fit<0.94 || fill<0.65 || area<w*h*0.15) continue;
            bool valid=true;
            for(int i=0;i<4;++i) {
                Point a=hull[(i+3)%4],b=hull[i],c=hull[(i+1)%4];
                const double ab=Distance(a,b),bc=Distance(b,c);
                if(ab<30 || bc<30 || std::abs((a.x-b.x)*(c.x-b.x)+(a.y-b.y)*(c.y-b.y))>0.85*ab*bc) valid=false;
            }
            if(!valid) continue;
            const double score=area*fit*fill;
            if(score>bestScore) {
                bestScore=score;
                // Clockwise from the upper-left corner. EXIF rotation has
                // already been applied by the image conversion pipeline.
                int first=0;
                for(int i=1;i<4;++i) if(hull[i].x+hull[i].y<hull[first].x+hull[first].y) first=i;
                for(int i=0;i<4;++i) best[i]={hull[(first+i)%4].x*source.GetWidth()/w,hull[(first+i)%4].y*source.GetHeight()/h};
            }
        }
    }
    if(bestScore==0) throw Error(L"未能可靠识别完整纸张边缘，已停止扫描，原图和上次合并文件未改动。\n请让一张浅色纸完整入镜，使用深色背景并避开遮挡；也可取消“图片扫描”保留整张图片。");
    return best;
}
}

std::unique_ptr<Gdiplus::Bitmap> ScanDocument(Gdiplus::Bitmap& source) {
    const Quad p=Detect(source);
    const double width=std::max(Distance(p[0],p[1]),Distance(p[3],p[2]));
    const double height=std::max(Distance(p[0],p[3]),Distance(p[1],p[2]));
    const double scale=std::min(1.0,3508.0/std::max(width,height));
    const int w=std::max(2,static_cast<int>(width*scale)),h=std::max(2,static_cast<int>(height*scale));
    // Projective map from a unit square to the detected quadrilateral.
    const double dx1=p[1].x-p[2].x,dx2=p[3].x-p[2].x,dx3=p[0].x-p[1].x+p[2].x-p[3].x;
    const double dy1=p[1].y-p[2].y,dy2=p[3].y-p[2].y,dy3=p[0].y-p[1].y+p[2].y-p[3].y;
    const double det=dx1*dy2-dx2*dy1;
    if(std::abs(det)<1e-8) throw Error(L"纸张角度过大，无法拉正，请重新拍摄。");
    const double g=(dx3*dy2-dx2*dy3)/det,j=(dx1*dy3-dx3*dy1)/det;
    const double a=p[1].x-p[0].x+g*p[1].x,b=p[3].x-p[0].x+j*p[3].x;
    const double d=p[1].y-p[0].y+g*p[1].y,e=p[3].y-p[0].y+j*p[3].y;
    std::vector<BYTE> gray(w*h);
    {
        Pixels input(source,Gdiplus::ImageLockModeRead);
        for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
            const double u=static_cast<double>(x)/(w-1),v=static_cast<double>(y)/(h-1),den=g*u+j*v+1;
            if(std::abs(den)<1e-8) throw Error(L"纸张透视校正失败。");
            const double sx=std::clamp((a*u+b*v+p[0].x)/den,0.0,static_cast<double>(source.GetWidth()-1));
            const double sy=std::clamp((d*u+e*v+p[0].y)/den,0.0,static_cast<double>(source.GetHeight()-1));
            const int x0=static_cast<int>(sx),y0=static_cast<int>(sy);
            const int x1=std::min(x0+1,static_cast<int>(source.GetWidth()-1)),y1=std::min(y0+1,static_cast<int>(source.GetHeight()-1));
            const double fx=sx-x0,fy=sy-y0;
            gray[y*w+x]=static_cast<BYTE>((1-fy)*((1-fx)*Gray(input.Row(y0)+3*x0)+fx*Gray(input.Row(y0)+3*x1))+
                                         fy*((1-fx)*Gray(input.Row(y1)+3*x0)+fx*Gray(input.Row(y1)+3*x1)));
        }
    }
    // Local mean illumination correction preserves grayscale strokes rather
    // than forcing every faint pen mark to pure black or white.
    const size_t stride=static_cast<size_t>(w)+1;
    std::vector<uint64_t> integral(stride*(h+1));
    for(int y=0;y<h;++y) {
        uint64_t sum=0;
        for(int x=0;x<w;++x) {sum+=gray[y*w+x];integral[(y+1)*stride+x+1]=integral[y*stride+x+1]+sum;}
    }
    auto result=std::make_unique<Gdiplus::Bitmap>(w,h,PixelFormat24bppRGB);
    if(result->GetLastStatus()!=Gdiplus::Ok) throw Error(L"内存不足，无法生成扫描页。");
    {
        Pixels output(*result,Gdiplus::ImageLockModeWrite);
        const int radius=std::max(15,std::min(w,h)/30);
        for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
            const int left=std::max(0,x-radius),right=std::min(w,x+radius+1);
            const int top=std::max(0,y-radius),bottom=std::min(h,y+radius+1);
            const auto sum=integral[bottom*stride+right]+integral[top*stride+left]-integral[top*stride+right]-integral[bottom*stride+left];
            const double mean=static_cast<double>(sum)/((right-left)*(bottom-top));
            const double normalized=gray[y*w+x]*245.0/std::max(1.0,mean);
            const BYTE value=static_cast<BYTE>(std::clamp((normalized-25)*255.0/210.0,0.0,255.0));
            BYTE* pixel=output.Row(y)+3*x; pixel[0]=pixel[1]=pixel[2]=value;
        }
    }
    return result;
}
}
