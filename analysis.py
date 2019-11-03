# -*-coding:utf-8 -*-

import matplotlib.pyplot as plt


def read_data():
    '''
    Read Data
    '''
    x = []
    y_hitrate = []
    y_totalio = []
    with open("./bin/data.out", mode="r", encoding="utf-8") as target:
        i = 1
        for line in target.readlines():
            x.append(i)
            comma = line.find(',')
            y_hitrate.append(float(line[0:comma]))
            y_totalio.append(int(line[comma + 1: len(line)]))
            i += 1
    return (x, y_hitrate, y_totalio)


x, y_hitrate, y_totalio = read_data()
plt.plot(x, y_hitrate)
plt.show()

plt.plot(x, y_totalio)
plt.show()
