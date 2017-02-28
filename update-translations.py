#!/usr/bin/python

##
##  Copyright (C) 2016-2017 Savoir-faire Linux Inc.
##
##  Author: Edric Milaret <edric.ladent-milaret@savoirfairelinux.com>
##  Author: Guillaume Roguez <guillaume.roguez@savoirfairelinux.com>
##  Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
##

import os
import shutil

print("== Pushing sources")
os.system("tx push -s")

print("== Pulling translations")
os.system("tx pull -af --minimum-perc=1")

print("== All done")