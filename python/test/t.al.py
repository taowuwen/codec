#!/usr/bin/env python3
# -*- coding: utf-8 -*-

def test_main():

    ai = [ a for a in range(10)]
    ke = [ a for a in range(10)]
    xue = [a for a in range(10)]

    dong = [ a for a in range(1, 10)]
    hao  = [ a for a in range(10)]
    re   = [ a for a in range(1, 10)]

    for a in ai:
        for k in ke:
            for x in xue:

                akx = a * 100 + k * 10 + x
                kx  = k * 10 + x

                dkx = akx * x
                kxh = akx * k
                rakx = akx * kx


                d = dkx // 100
                k1 = kxh // 100
                h = kxh % 10
                r = rakx // 1000

                rakx1 = r * 1000 + a * 100 + k * 10 + x
                kxh1 = k * 100 + x * 10 + h


                if d in dong and k1 == k and h in hao and r in re and rakx1 == rakx and kxh1 == kxh:
                    print("爱：{}, 科: {}, 学: {}, 懂: {}, 好: {}, 热: {}, [ {} * {} = {}, dkx: {}, kxh: {}]".format(a, k, x, d, h, r, akx, kx, rakx, dkx, kxh))

if __name__ == '__main__':
    print("hello, here is the answer below: ")
    test_main()
