# -*- coding: utf-8 -*-
"""Copie allégée d'une texture pour l'éditeur de salles en ligne : WebP, 1024 px au plus (l'éditeur
n'affiche pas plus grand). Appelé par eon_salles.publish_drive avec le Python du système (Pillow),
le Python d'Unreal n'ayant pas Pillow.   py -3 texture_web.py <source.png> <cible.webp>"""
import sys
from PIL import Image

im = Image.open(sys.argv[1])
im.thumbnail((1024, 1024), Image.LANCZOS)
im.save(sys.argv[2], "WEBP", quality=85, method=6)
