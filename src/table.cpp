/*
** table.cpp
**
** This file is part of mkxp.
**
** Copyright (C) 2013 Jonas Kulla <Nyocurio@gmail.com>
**
** mkxp is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 2 of the License, or
** (at your option) any later version.
**
** mkxp is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with mkxp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "table.h"

#include <string.h>
#include <algorithm>

#include "exception.h"
#include "util.h"

/* Init normally */
Table::Table(int x, int y /*= 1*/, int z /*= 1*/)
    : xs(x), ys(y), zs(z),
      data(x*y*z)
{}

Table::Table(const Table &other)
    : xs(other.xs), ys(other.ys), zs(other.zs),
      data(other.data)
{}

int16_t Table::get(int x, int y, int z) const
{
	return data[xs*ys*z + xs*y + x];
}

void Table::set(int16_t value, int x, int y, int z)
{
	if (x < 0 || x >= xs
	||  y < 0 || y >= ys
	||  z < 0 || z >= zs)
	{
		return;
	}

	data[xs*ys*z + xs*y + x] = value;

	modified();
}

void Table::resize(int x, int y, int z)
{
	if (x == xs && y == ys && z == zs)
		return;

	std::vector<int16_t> newData(x*y*z);

	for (int k = 0; k < std::min(z, zs); ++k)
		for (int j = 0; j < std::min(y, ys); ++j)
			for (int i = 0; i < std::min(x, xs); ++i)
				newData[x*y*k + x*j + i] = at(i, j, k);

	data.swap(newData);

	xs = x;
	ys = y;
	zs = z;

	return;
}

void Table::resize(int x, int y)
{
	resize(x, y, zs);
}

void Table::resize(int x)
{
	resize(x, ys, zs);
}

/* Serializable */
int Table::serialSize() const
{
	/* header + data */
	return 20 + (xs * ys * zs) * 2;
}

void Table::serialize(mkxp::serializer ss) const
{
	/* Table dimensions: we don't care
	 * about them but RMXP needs them */
	int dim = 1;
	int size = xs * ys * zs;

	if (ys > 1)
		dim = 2;

	if (zs > 1)
		dim = 3;

  ss.write_one<int32_t>(dim);
  ss.write_one<int32_t>(xs);
  ss.write_one<int32_t>(ys);
  ss.write_one<int32_t>(zs);
  ss.write_one<int32_t>(size);

  ss.write_many<int16_t>(data);
}


Table *Table::deserialize(mkxp::deserializer ds)
{
	if (ds.available_bytes() < 20)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	ds.read_one<int32_t>();

	int x = ds.read_one<int32_t>();
	int y = ds.read_one<int32_t>();
	int z = ds.read_one<int32_t>();
	int size = ds.read_one<int32_t>();

	if (size != x*y*z)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	if (ds.available_bytes() != 2*size)
		throw Exception(Exception::RGSSError, "Marshal: Table: bad file format");

	Table *t = new Table(x, y, z);
  if(!t->data.empty())
    ds.read_many<int16_t>(t->data);

	return t;
}
