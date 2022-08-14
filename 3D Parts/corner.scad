CORNER_THICKNESS = 13.0;
SCREW_DIAMETER = 3.5;
SCREW_WALL_THICKNESS = (14 - SCREW_DIAMETER) / 2;
DETAIL = 32;
TAB_THICKNESS = 3.175;
TAB_TOLERANCE = 0.1;
TAB_WALL_THICKNESS = 2;
TAB_LENGTH = 30.0;
SNAP_DIAMETER = 4;
SNAP_HEIGHT = TAB_THICKNESS / 3;
SNAP_OFFSET_FROM_CORNER = 30;
REGISTRATION_TAB_LENGTH = 10;
REGISTRATION_TAB_DEPTH = 3.175;

// Calculated Values
OUTER_CYLINDER = SCREW_DIAMETER / 2 + SCREW_WALL_THICKNESS;
INNER_BOX_THICKNESS = TAB_THICKNESS + TAB_TOLERANCE * 2;
OUTER_BOX_THICKNESS = INNER_BOX_THICKNESS + TAB_WALL_THICKNESS * 2;
OUTER_BOX_WALL_THICKNESS = (OUTER_BOX_THICKNESS - INNER_BOX_THICKNESS) / 2;

module screwhole()
{
	translate(v = [OUTER_CYLINDER, OUTER_CYLINDER, 0])
		difference()
		{
			cylinder(r = OUTER_CYLINDER, h = CORNER_THICKNESS, $fn = DETAIL);
			translate(v = [0, 0, -1])
				cylinder(r = SCREW_DIAMETER / 2, h = CORNER_THICKNESS + 2, $fn = DETAIL);
		}
}

module tab()
{
	union()
	{
		translate(v = [0, SCREW_DIAMETER / 2 + SCREW_WALL_THICKNESS, 0])
			difference()
			{
				cube(size = [OUTER_BOX_THICKNESS, TAB_LENGTH, CORNER_THICKNESS]);
				translate(v = [TAB_WALL_THICKNESS, (OUTER_BOX_THICKNESS - INNER_BOX_THICKNESS) / 2, -1])
					cube(size = [INNER_BOX_THICKNESS, TAB_LENGTH, CORNER_THICKNESS + 2]);

			}
		snap1();
		snap2();
	}
}

module snap1()
{
	translate(v = [OUTER_BOX_WALL_THICKNESS - 0.01, SNAP_OFFSET_FROM_CORNER, CORNER_THICKNESS / 2])
		difference()
		{
			rotate(a = [0, 90, 0])
				cylinder(r = SNAP_DIAMETER / 2, h = SNAP_HEIGHT, $fn = DETAIL);
			translate(v = [0, SNAP_DIAMETER / 2, 0])
				rotate(a = [0, 0, 15])
					translate(v = [0, SNAP_DIAMETER * -1, SNAP_DIAMETER / -2])
						cube(size = [SNAP_DIAMETER, SNAP_DIAMETER, SNAP_DIAMETER]);
		}
}

module snap2()
{
	translate(v = [OUTER_BOX_WALL_THICKNESS + INNER_BOX_THICKNESS + 0.01, SNAP_OFFSET_FROM_CORNER, CORNER_THICKNESS / 2])
		scale(v = [-1, 1, 1])
			difference()
			{
				rotate(a = [0, 90, 0])
					cylinder(r = SNAP_DIAMETER / 2, h = SNAP_HEIGHT, $fn = DETAIL);
				translate(v = [0, SNAP_DIAMETER / 2, 0])
					rotate(a = [0, 0, 15])
						translate(v = [0, SNAP_DIAMETER * -1, SNAP_DIAMETER / -2])
							cube(size = [SNAP_DIAMETER, SNAP_DIAMETER, SNAP_DIAMETER]);
			}
}


module tabs()
{
	difference()
	{
		union()
		{
			tab();
			translate(v = [0, OUTER_BOX_THICKNESS, 0])
			rotate(a = [0, 0, -90])
				tab();
		}
		translate(v = [OUTER_CYLINDER, OUTER_CYLINDER, -1])
			cylinder(r = SCREW_DIAMETER / 2, h = CORNER_THICKNESS + 2, $fn = DETAIL);
	}
}

module strain_reliefs()
{
	difference()
	{
		for (z_pos = [0 : 2])
		{
			translate(v = [OUTER_BOX_THICKNESS - 0.01, OUTER_BOX_THICKNESS - 0.01, CORNER_THICKNESS / 5 * z_pos * 2])
				intersection()
				{
					cylinder(r = OUTER_CYLINDER * 2, h = CORNER_THICKNESS / 5);
					cube(size = [OUTER_CYLINDER * 2, OUTER_CYLINDER * 2, CORNER_THICKNESS]);
				}
		}
		translate(v = [OUTER_CYLINDER, OUTER_CYLINDER, -1])
			cylinder(r = SCREW_DIAMETER / 2, h = CORNER_THICKNESS + 2, $fn = DETAIL);
	}
}

module registration_tabs()
{
	union()
	{
		translate(v = [0, TAB_LENGTH + SCREW_DIAMETER / 2 + SCREW_WALL_THICKNESS - REGISTRATION_TAB_LENGTH, CORNER_THICKNESS - 0.01])
			cube(size = [OUTER_BOX_WALL_THICKNESS, REGISTRATION_TAB_LENGTH, REGISTRATION_TAB_DEPTH]);
		translate(v = [TAB_LENGTH + SCREW_DIAMETER / 2 + SCREW_WALL_THICKNESS - REGISTRATION_TAB_LENGTH, 0, CORNER_THICKNESS - 0.01])
			cube(size = [REGISTRATION_TAB_LENGTH, OUTER_BOX_WALL_THICKNESS, REGISTRATION_TAB_DEPTH]);
	}
}

union()
{
	screwhole();
	tabs();
	strain_reliefs();
	registration_tabs();
	//snap1();
	//snap2();
}
