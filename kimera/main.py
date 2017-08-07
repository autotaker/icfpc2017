import json
from sklearn import linear_model
import click


def learn(X, y):
    reg = linear_model.LinearRegression()
    reg.fit(X, y)
    return reg


def average(xs):
    if len(xs) < 3:
        return None
    n = 0.0
    m = 0.0
    r = 0.98
    k = 1.0
    for x in (xs):
        n += x * k
        m += k
        k *= r
    return n / m


data = json.load(open('summary.json'))

ais = set()
maps = set()
for item in data:
    ais.add(item['ai_id'])
    maps.add(item['map_id'])
ais = sorted(list(ais))
maps = sorted(list(maps))


ranks = {ai: {map: [] for map in maps} for ai in ais}
for item in data:
    ai = item['ai_id']
    map = item['map_id']
    rank = item['rank']
    rank = 1.0 - (rank - 1) / (item['punters'] - 1)
    ranks[ai][map].append(rank)

values = {ai: {map: [] for map in maps} for ai in ais}
for map in maps:
    for ai in ais:
        values[ai][map] = average(ranks[ai][map])

features = {map: None for map in maps}
for line in open('maps.csv'):
    fs = line.split(' ')
    features[fs[0]] = [float(x) for x in fs[1:]]


@click.group()
def main():
    pass


@main.command()
@click.option('--test-maps', '-M', multiple=True)
@click.option('--ais', '-A', multiple=True)
def test(test_maps, ais):

    models = {}

    for ai in ais:

        X = []
        y = []
        for map in maps:
            x = features[map]
            val = values[ai][map]
            if x is None or val is None:
                continue
            X.append(x)
            y.append(val)

        models[ai] = learn(X, y)

    for map in test_maps:
        X = [features[map]]
        print()
        print('#', map)
        for ai in ais:
            y = models[ai].predict(X)[0]
            print(ai, y)


@main.command()
@click.option('--test-maps', '-M', multiple=True)
@click.option('--ais', '-A', multiple=True)
def eval(test_maps, ais):
    for ai in ais:
        X = []
        y = []
        X_test = []
        y_test = []
        for map in maps:
            x = features[map]
            val = values[ai][map]
            if x is None or val is None:
                continue
            if map not in test_maps:
                X.append(x)
                y.append(val)
            else:
                X_test.append(x)
                y_test.append(val)

        if len(X) == 0 or len(X_test) == 0:
            continue

        reg = learn(X, y)

        print('+++')
        print('#', ai)
        predicted_y = reg.predict(X_test)
        print('Actual   ', y_test)
        print('Predicted', predicted_y)
        err = 0
        for i in range(len(X_test)):
            err += abs(predicted_y[i] - y_test[i])
        err /= len(X_test)
        print('abs error', err)


@main.command()
@click.option('--ais', '-A', multiple=True)
def train(ais):

    models = {}

    for ai in ais:

        X = []
        y = []
        for map in maps:
            x = features[map]
            val = values[ai][map]
            if x is None or val is None:
                continue
            X.append(x)
            y.append(val)

        models[ai] = learn(X, y)

    for ai in ais:
        print("{},".format(ai))

    print("""
    vector<float> b = {{
        {}
    }};
    """
    .format(','.join(str(models[ai].intercept_) for ai in ais)))

    print("""
    vector<vector<float>> w = {{
        {}
    }};
    """
    .format(',\n'.join('{' + ','.join((str(x) for x in models[ai].coef_)) + '}' for ai in ais)))


main()
