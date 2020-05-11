#include <iostream>
#include <opencv2/core.hpp>
#include <cmath>
#include "debug.h"

using namespace cv;
using namespace std;

struct pairs_of_parallel_lines {
    int pair11 = 0;
    int pair12 = 1;
    int pair21 = 2;
    int pair22 = 3;
};

struct lineABC {
    double A, B, C;
};

struct line_result {
    double k, bmin, bmax, b, MSD;
    bool flag = true;
};

class MSD {

private:
    double error_fmin;
    double error_MSD;
    int max_counter;
    double count;

public:
    MSD(double error_fmin_Init, double error_MSD_Init, int max_counter_Init, double count_Init) {
        error_fmin = error_fmin_Init;
        error_MSD = error_MSD_Init;
        max_counter = max_counter_Init;
        count = count_Init;
    }

private:
    static void find_pairs(line_result res[4], pairs_of_parallel_lines &result) {
        double k = res[0].k;
        double min = k;
        for (int i = 1; i <= 3; i++) {
            double a = abs(k - res[i].k);
            if (a < min) {
                result.pair12 = i;
                min = a;
            }
        }
        if (result.pair12 == 2) {
            result.pair21 = 1;
            result.pair22 = 3;
        } else if (result.pair12 == 3) {
            result.pair21 = 1;
            result.pair22 = 2;
        }
    }

    void find_min_MSD_para(line_result (&res)[4], int pair1, int pair2, vector<vector<pair<double, double>>> array_of_points) {
        double result = 0.0;
        double result_last = result + 2 * error_MSD;
        int counter = 0;
        double err = abs(result_last - result);
        double b1 = (res[pair1].bmin + res[pair1].bmax) / 2;
        double b2 = (res[pair2].bmin + res[pair2].bmax) / 2;
        double k = (res[pair1].k + res[pair2].k) / 2;
        while (err >= error_MSD && counter <= max_counter) {
             result = f_min_para(res[pair1].bmin, res[pair1].bmax, res[pair2].bmin,
                 res[pair2].bmax, k, b1, b2, array_of_points[pair1], array_of_points[pair2]);
             err = abs(result_last - result);
             result_last = result;
             counter++;
        }
        res[pair1].b = b1;
        res[pair2].b = b2;
        res[pair1].MSD = sqrt(result);
        res[pair2].MSD = sqrt(result);
        if (abs(k) >= 1.0e5) {
            res[pair1].flag = false;
            res[pair2].flag = false;
        } else {
            res[pair1].k = k;
            res[pair2].k = k;
        }
    }

    static double foo_fmin_para(double &k, double sum, double b1, double b2,
            vector<pair<double, double>> points1, vector<pair<double, double>> points2) {
        int size1 = points1.size();
        int size2 = points2.size();
        double k1 = 0.0;
        double k2 = 0.0;
        for (int i = 0; i < size1; i++)
            k1 += points1[i].first * (points1[i].second - b1);
        for (int j = 0; j < size2; j++)
            k2 += points2[j].first * (points2[j].second - b2);
        k = (k1 + k2) / sum;
        if (abs(k) > 1.0e5 || sum == 0) k = 1.0e5;
        k1 = 0.0;
        k2 = 0.0;
        for (int i = 0; i < size1; i++) {
            if (k < 1.0e5) k1 += pow((points1[i].second - k * points1[i].first - b1), 2);
            else k1 += pow((points1[i].first - b1), 2);
        }
        for (int j = 0; j < size2; j++) {
            if (k < 1.0e5) k2 += pow((points2[j].second - k * points2[j].first - b2), 2);
            else k2 += pow((points2[j].first - b2), 2);
        }
        return (k1 + k2);
    }

    double f_min_para(double bmin1, double bmax1, double bmin2, double bmax2, double &k, double &b1, double &b2,
            vector<pair<double, double>> points1, vector<pair<double, double>> points2) {
        double sum = 0.0;
        double Lcenter, Rcenter, Lfcenter, Rfcenter, beg, end, BegBuf, EndBuf;
        for (int i = 0; i < points1.size(); i++)
            sum += pow(points1[i].first, 2);
        for (int j = 0; j < points2.size(); j++)
            sum += pow(points2[j].first, 2);
        double delta = (sqrt(5) - 1) / 2;
        for (int j = 0; j <= 1; j++) {
            if (j == 0) {
                beg = bmin1;
                end = bmax1;
                BegBuf = foo_fmin_para(k, sum, bmin1, b2, points1, points2);
                EndBuf = foo_fmin_para(k, sum, bmax1, b2, points1, points2);
            } else {
                beg = bmin2;
                end = bmax2;
                BegBuf = foo_fmin_para(k, sum, b1, bmin2, points1, points2);
                EndBuf = foo_fmin_para(k, sum, b1, bmax2, points1, points2);
            }
            for (int i = 0; i <= max_counter; i++) {
                double g = delta * (end - beg);
                Lcenter = end - g;
                if (j == 0) Lfcenter = foo_fmin_para(k, sum, Lcenter, b2, points1, points2);
                    else Lfcenter = foo_fmin_para(k, sum, b1, Lcenter, points1, points2);
                Rcenter = beg + g;
                if (j == 0) Rfcenter = foo_fmin_para(k, sum, Rcenter, b2, points1, points2);
                    else Rfcenter = foo_fmin_para(k, sum, b1, Rcenter, points1, points2);
                if (Lfcenter < Rfcenter) {
                    end = Rcenter;
                    EndBuf = Rfcenter;
                } else if (Lfcenter > Rfcenter) {
                    beg = Lcenter;
                    BegBuf = Lfcenter;
                } else {
                    end = Rcenter;
                    EndBuf = Rfcenter;
                    beg = Lcenter;
                    BegBuf = Lfcenter;
                }
                double r = fabs(end - beg);
                if (r < error_fmin) {
                    double f = (end + beg) / 2;
                    if (j == 0) b1 = f;
                    else b2 = f;
                    break;
                }
            }
        }
        return (foo_fmin_para(k, sum, b1, b2, points1, points2));
    }

    void find_min_MSD_mono(line_result (&res)[4], int number,
                vector<vector<pair<double, double>>> array_of_points) {
        double b = (res[number].bmin + res[number].bmax) / 2;
        double result = 0.0;
        double result_last = result + 2 * error_MSD;
        int counter = 0;
        double err = abs(result_last - result);
        while (err >= error_MSD && counter <= max_counter) {
            result = f_min_mono(res[number].bmin, res[number].bmax, res[number].k, b, array_of_points[number]);
            err = abs(result_last - result);
            result_last = result;
            counter++;
        }
        if (abs(res[number].k) >= 1.0e5) res[number].flag = false;
        res[number].b = b;
        res[number].MSD = sqrt(result);
    }

   static double foo_fmin_mono(double &k, double sum, double b, vector<pair<double, double>> points) {
        int size = points.size();
        int i;
        double k1 = 0.0;
        for (i = 0; i < size; i++) 
            k1 += points[i].first * (points[i].second - b);
        k = k1 / sum;
        if (abs(k) > 1.0e5 || sum == 0) k = 1.0e5;
        k1 = 0.0;
        for (i = 0; i < size; i++) {
            if (k < 1.0e5) k1 += pow((points[i].second - k * points[i].first - b), 2);
            else k1 += pow((points[i].first - b), 2);
        }
        return k1;
    }

    double f_min_mono(double bmin, double bmax, double &k, double &b, vector<pair<double, double>> points) {
        double sum = 0.0;
        for (int i = 0; i < points.size(); i++)
            sum += pow(points[i].first, 2);
        double Lcenter, Rcenter, Lfcenter, Rfcenter, beg, end, BegBuf, EndBuf;
        double delta = (sqrt(5) - 1) / 2;
        beg = bmin;
        end = bmax;
        BegBuf = foo_fmin_mono(k, sum, beg, points);
        EndBuf = foo_fmin_mono(k, sum, end, points);
        for (int i = 0; i <= max_counter; i++) {
            double g = delta * (end - beg);
            Lcenter = end - g;
            Lfcenter = foo_fmin_mono(k, sum, Lcenter, points);
            Rcenter = beg + g;
            Rfcenter = foo_fmin_mono(k, sum, Rcenter, points);
            if (Lfcenter < Rfcenter) {
                end = Rcenter;
                EndBuf = Rfcenter;
            } else if (Lfcenter > Rfcenter) {
                beg = Lcenter;
                BegBuf = Lfcenter;
            } else {
                end = Rcenter;
                EndBuf = Rfcenter;
                beg = Lcenter;
                BegBuf = Lfcenter;
            }
            double r = fabs(end - beg);
            if (r < error_fmin) {
                b = (end + beg) / 2;
                break;
            }
        }
        return (foo_fmin_mono(k, sum, b, points));
    }

    static vector<pair<double, double>> intersection(line_result res[4], int pair1) {
        vector<pair<double, double>> result;
        int f = 0;
        double x, y;
        for (int j = 0; j <= 1; j++) {
            for (int i = 1; i <= 3; i++) {
                if (i != pair1) {
                    if (res[i].flag == false) {
                        x = res[i].b;
                        y = res[f].k * x + res[f].b;
                    } else if (res[f].flag == false) {
                        x = res[f].b;
                        y = res[i].k * x + res[i].b;
                    } else {
                        x = (res[i].b - res[f].b) / (res[f].k - res[i].k);
                        y = res[f].k * x + res[f].b;
                    }
                    result.push_back(make_pair(x, y));
                }
            }
            f = pair1;
        }
        return result;
    }

    static vector<pair<double, double>> comparison(line_result res_para[4], line_result res_mono[4],
                pairs_of_parallel_lines squad_pairs, vector<pair<double, double>> result_para,
                vector<pair<double, double>> result_mono, double &error_result) {
        vector<pair<double, double>> result;
        int pair_first = squad_pairs.pair11;
        int pair_second = squad_pairs.pair21;
        for (int i = 0; i <= 1; i++) {
           double compare = res_para[pair_first].MSD / (res_mono[pair_first].MSD + res_mono[pair_second].MSD);
           printf(" Compare = MSD_para / (MSD_mono1 + MSD_mono2) = %f\n", compare);
           if (compare > 1.5) {
                result.push_back(make_pair(result_mono[pair_first].first, result_mono[pair_first].second));
                printf("\t quadrangle\n");
                result.push_back(make_pair(result_mono[pair_second].first, result_mono[pair_second].second));
                error_result += (res_mono[pair_first].MSD + res_mono[pair_second].MSD);
           } else {
                result.push_back(make_pair(result_para[pair_first].first, result_para[pair_first].second));
                printf("\t parallelogram\n");
                result.push_back(make_pair(result_para[pair_second].first, result_para[pair_second].second));
                error_result += res_para[pair_first].MSD;
           }
           pair_first = squad_pairs.pair21;
           pair_second = squad_pairs.pair22;
        }
        return result;
    }

 public:
     vector<pair<double, double>> MSD_main(vector<vector<pair<double, double>>> Points, 
             vector<lineABC> Lines, double &error_result) {
        line_result res_para[4], res_mono[4];
        int i;
        for (i = 0; i <= 3; i++) 
            if (abs(Lines[i].B) <= 0.01) res_para[i].flag = false;
        for (i = 0; i <= 3; i++) {
            double bsr;
            if (res_para[i].flag == true) {
                bsr = -Lines[i].C / Lines[i].B;
                res_para[i].k = -Lines[i].A / Lines[i].B;
            } else {
                bsr = -Lines[i].C / Lines[i].A;
                res_para[i].k = 1.0e5;
            }
            double log = log10(abs(bsr));
            int f = floor(log);
            double g = count * pow(10, -f);
            if (log - f < 0.3) g *= 5;
            if (log - f < 0.15) g *= 3;
            if (log - f > 0.7) g *= 0.8;
            if (log - f > 0.85) g *= 0.7;
            res_para[i].bmin = bsr - g * bsr;
            res_para[i].bmax = bsr + g * bsr;
            res_mono[i] = res_para[i];
        }
        /*!*/ for (i = 0; i <= 3; i++)
        /*!*/    printf("\t k = %f\t bmin = %f\t bmax = %f\n", res_para[i].k, res_para[i].bmin, res_para[i].bmax);
        pairs_of_parallel_lines squad_pairs;
        find_pairs(res_para, squad_pairs);
        find_min_MSD_para(res_para, squad_pairs.pair11, squad_pairs.pair12, Points);
        find_min_MSD_para(res_para, squad_pairs.pair21, squad_pairs.pair22, Points);
        for (i = 0; i <= 3; i++) 
            find_min_MSD_mono(res_mono, i, Points);
        vector<pair<double, double>> result_para = intersection(res_para, squad_pairs.pair12);
        vector<pair<double, double>> result_mono = intersection(res_mono, squad_pairs.pair12);
        /*!*/ for (i = 0; i <= 3; i++)
        /*!*/    printf(" k_para = %f\t b_para = %f\t MSD_para = %f\n", res_para[i].k, res_para[i].b, res_para[i].MSD);
        /*!*/ for (i = 0; i <= 3; i++)
        /*!*/   printf("\t k_mono = %f\t b_mono = %f\t MSD_mono = %f\n", res_mono[i].k, res_mono[i].b, res_mono[i].MSD);
        vector<pair<double, double>> result_full = 
            comparison(res_para, res_mono, squad_pairs, result_para, result_mono, error_result);
        return result_full;
     }
};