//author: chenliang
//date:2019/5/1
#include <algorithm>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "SimpleCV.h"

#ifndef DISABLE_SIMPLE_CV

namespace cv
{
    template<typename _Tp> Point_<_Tp>::Point_() : x(0), y(0)
    {
    }
    template<typename _Tp> Point_<_Tp>::Point_(_Tp _x, _Tp _y) : x(_x), y(_y)
    {
    }
    template<typename _Tp> Point_<_Tp> Point_<_Tp>::operator-(const Point_<_Tp>& in) const
    {
        return Point_<_Tp>(x - in.x, y - in.y);
    }

    template class Point_<int>;
    template class Point_<float>;

    Mat::Mat(int _rows, int _cols) : rows(_rows), cols(_cols)
    {
        assert(_rows * _cols <= MAT_MAX_ITEMS);
        memset(m, 0, sizeof(float) * MAT_MAX_ITEMS);
    }
    Mat::Mat(int _rows, int _cols, float *_m) : rows(_rows), cols(_cols)
    {
        assert(_rows * _cols <= MAT_MAX_ITEMS);
        memcpy(m, _m, sizeof(float) * _rows * _cols);
    }
    float &Mat::at(int _rows, int _cols)
    {
        return m[_rows * cols + _cols];
    }
    const float &Mat::at(int _rows, int _cols) const
    {
        return m[_rows * cols + _cols];
    }
    Mat Mat::operator*(Mat const& rhs) const
    {
        assert(cols == rhs.rows);
        Mat ret(rows, rhs.cols);
        for (size_t i = 0; i < rows; i++)
        {
            for (size_t j = 0; j < rhs.cols; j++)
            {
                float sum = 0;
                for (size_t k = 0; k < cols; k++)
                {
                    sum += ((Mat*)this)->at(i, k) * ((Mat*)&rhs)->at(k, j);
                }
                ret.at(i, j) = sum;
            }
        }
        return ret;
    }

    float norm(const Point2f &p)
    {
        return sqrt(p.x*p.x + p.y*p.y);
    }
}

#endif
