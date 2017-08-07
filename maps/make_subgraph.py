import json
import click
import random


class hd(dict):
    def __hash__(self):
        return hash(tuple(sorted(self.items())))


@click.command()
@click.argument('map', default='nara-scaled.json')
@click.argument('size', type=click.Choice(['small', 'medium', 'large']))
def main(map, size):

    vertex = {
        'small': 40,
        'medium': 300,
        'large': 2000
    }

    num_mines = random.randrange(4) + 2


    org = json.load(open(map))


    neigh_rivers = {}
    for river in org['rivers']:
        s = river['source']
        t = river['target']
        if s not in neigh_rivers:
            neigh_rivers[s] = []
        if t not in neigh_rivers:
            neigh_rivers[t] = []
        neigh_rivers[s].append(river)
        neigh_rivers[t].append(river)


    id2site = {}
    for site in org['sites']:
        id2site[site['id']] = site


    sites = set()
    root = random.choice(org['sites'])
    sites.add(hd(root))

    while len(sites) < vertex[size]:
        site = random.choice(list(sites))
        u = site['id']
        river = random.choice(neigh_rivers[u])
        v = river['target']
        sites.add(hd(id2site[river['source']]))
        sites.add(hd(id2site[river['target']]))

    sites = list(sites)
    used = set()
    for site in sites:
        used.add(site['id'])

    rivers = []
    for river in org['rivers']:
        if river['source'] in used and river['target'] in used:
            rivers.append(river)


    mines = random.sample(list(used), num_mines)

    print(json.dumps({'sites': sites, 'rivers': rivers, 'mines': mines}))



main()
