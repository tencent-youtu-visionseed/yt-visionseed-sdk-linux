//author: chenliang
//date:2019/5/1


#ifndef _SIMPLE_CV_H_
#define _SIMPLE_CV_H_

#ifndef DISABLE_SIMPLE_CV

#define MAT_MAX_ITEMS 40
namespace cv
{
    template<typename _Tp> class Point_
    {
    public:
        Point_();
        Point_(_Tp _x, _Tp _y);
        _Tp x;
        _Tp y;

        Point_<_Tp> operator-(const Point_<_Tp>& in) const;
    };

    class Mat
    {
    protected:
        float m[MAT_MAX_ITEMS];
    public:
        int rows;
        int cols;

        Mat(int _rows, int _cols);
        Mat(int _rows, int _cols, float *_m);
        float &at(int _rows, int _cols);
        const float &at(int _rows, int _cols) const;

        Mat operator*(Mat const& rhs) const;
    };

    // typedef Point_<int> Point2i;
    // typedef Point_<int64> Point2l;
    typedef Point_<float> Point2f;
    typedef Point_<int> Size;
    // typedef Point_<double> Point2d;

    float norm(const Point2f &p);
}

#endif  //DISABLE_SIMPLE_CV
#endif  //_SIMPLE_CV_H_
