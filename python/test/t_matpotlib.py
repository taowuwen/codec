#!/usr/bin/env python3



if __name__ == '__main__':
    import numpy as np
    import matplotlib.pyplot as plt

    x = np.arange(0, 2 * np.pi, 0.1)
    y = np.sin(x)

    plt.plot(x, y)
    plt.show()

