/*
 * geoference.c
 *
 *  Created on: 8. 5. 2025
 *      Author: vojtech
 */

#include "geoference.h"

/*
 FREQUENCIES:
 Europe				144.800
 Russia				144.800
 South Africa		144.800
 North America		144.390
 Mexico				144.390
 Colombia			144.390
 Chile				144.390
 Indonesia			144.390
 Malaysia			144.390
 Thailand			145.525
 Panama				145.010
 Costa Rica			145.010
 Venezuela			145.010
 Nicaragua			145.010
 Brazil				145.570
 Argentina			144.930
 Paraguay			144.930
 Uruguay			144.930
 China				144.640
 Japan				144.660
 South Korea		144.620
 New Zealand		144.575
 Australia			145.175

 NO APRS:
 Latvia
 North Korea
 United Kingdom
 Yemen
 */

static float arg_par_uru[] =
{
		-67.2363281, -22.9179229,
		-70.8398438, -36.7388841,
		-72.8613281, -50.7364551,
		-68.6425781, -52.4827802,
		-68.5546875, -55.0280221,
		-56.8652344, -54.4189300,
		-52.4267578, -33.2478759,
		-57.2607422, -30.1451272,
		-53.3496094, -26.9024769,
		-58.6669922, -19.3111434,
		-61.9189453, -19.9320413,
		-62.5341797, -22.2280904,
		-67.2363281, -22.9179229
};

static float aus[] =
{
		144.3164063, -9.4490618,
		102.7441406, -14.5197800,
		111.9726563, -45.6447682,
		157.5000000, -46.6795945,
		158.5546875, -21.4530686,
		144.4042969, -9.4490618
};

static float bra[] =
{
		-52.4267578, -33.2478759,
		-25.4003906, -9.9688506,
		-49.4824219, 2.8113712,
		-62.9296875, 4.5654736,
		-75.4980469, -6.7518965,
		-53.3496094, -26.9024769,
		-57.2607422, -30.1451272,
		-52.4267578, -33.2478759
};

static float chi[] =
{
		87.1875000, 49.3824000,
		77.6513672, 42.7470122,
		73.1250000, 39.1982053,
		81.5185547, 29.9168522,
		88.8574219, 27.6835281,
		95.4052734, 28.6905877,
		99.4921875, 22.3500758,
		109.1162109, 17.6021391,
		122.6953125, 22.0245456,
		123.0908203, 38.3071806,
		131.3964844, 42.6501218,
		136.3183594, 48.2246726,
		121.7285156, 54.9271419,
		104.5898438, 42.1634034,
		87.1875000, 49.3824000
};

static float cos_nic_pan[] =
{
		-88.6376953, 12.1252642,
		-79.6289063, 5.1347146,
		-76.2011719, 9.2105601,
		-82.9467773, 16.2146746,
		-88.6376953, 12.1252642
};

static float jap[] =
{
		140.4492188, 45.8900082,
		131.4404297, 36.7740925,
		127.4414063, 32.6208702,
		126.2329102, 25.7207351,
		142.4707031, 23.3220800,
		155.3906250, 49.9512199,
		140.4492188, 45.8900082
};

static float lat[] =
{
		26.6418000, 55.6838000,
		28.2238770, 56.2280850,
		27.7349854, 57.4272096,
		26.4221191, 57.6395208,
		25.2465820, 58.0662560,
		21.6485596, 57.7598683,
		20.8850098, 56.9809114,
		20.7421875, 56.0904271,
		22.1044922, 56.4230166,
		25.2136230, 56.1975370,
		26.5869141, 55.6713893
};

static float new_zel[] =
{
		188.5693359, -39.7747695,
		171.7822266, -28.4590330,
		156.2255859, -44.0560117,
		173.8037109, -53.0676266,
		188.5693359, -39.7747695
};

static float nor_kor[] =
{
		123.7829590, 39.9434365,
		124.5410156, 37.4574181,
		129.0124512, 38.7112325,
		127.8259277, 39.4955634,
		131.1547852, 41.9921602,
		129.7595215, 43.2291951,
		123.7829590, 39.9518589
};

static float sou_kor[] =
{
		124.5410156, 37.4574181,
		129.0124512, 38.7112325,
		129.9023438, 35.4248679,
		126.0571289, 32.5282894,
		124.5849609, 37.4399741
};

static float UK[] =
{
		-0.5932617, 61.0263703,
		-7.9541016, 58.2228110,
		-7.9650879, 54.1752967,
		-5.0427246, 53.8913913,
		-6.4819336, 49.7670741,
		1.4941406, 50.9791824,
		1.8457031, 52.6430634,
		-2.0983887, 56.3287209,
		-0.5932617, 61.0263703
};

static float ven[] =
{
		-70.6764221, 12.7756430,
		-73.5424805, 9.0153023,
		-71.8066406, 6.7955350,
		-67.6757813, 6.1187077,
		-66.6430664, 1.0765968,
		-63.6547852, 1.9112665,
		-64.8632813, 4.0615356,
		-60.7324219, 4.7187776,
		-59.5239258, 9.3623528,
		-70.6764221, 12.7756430
};

static float yem[] =
{
		52.1081543, 19.3111434,
		42.0556641, 17.3401517,
		43.4838867, 12.1360052,
		53.8330078, 15.6653542,
		52.1081543, 19.3111434
};

static float tha[] =
{
		112.1484375, 15.1145529,
		97.6904297, 23.2817192,
		97.2070313, 5.9220446,
		104.1503906, 1.0546279,
		112.1484375, 15.1145529
};


bool point_in_polygon(uint32_t num_of_poly_corners, float *polygon, float latitude, float longitude)
{
	uint32_t i = 0;
	uint32_t j = num_of_poly_corners * 2 - 2;
	bool odd_nodes = false;

	for(i = 0; i < num_of_poly_corners * 2; i++)
	{
		if(((polygon[i + 1] < latitude && polygon[j + 1] >= latitude) || (polygon[j + 1] < latitude && polygon[i + 1] >= latitude)) && (polygon[i] <= longitude || polygon[j] <= longitude))
		{
			odd_nodes ^= (polygon[i] + (latitude - polygon[i + 1]) / (polygon[j + 1] - polygon[i + 1]) * (polygon[j] - polygon[i]) < longitude);
		}
		j = i;
	}

	return odd_nodes;
}

void set_APRS_freq(float latitude, float longitude)
{
	//EU, africa
	if(longitude > -40.0 && longitude < 70.0)
	{
		if(point_in_polygon(9, UK, latitude, longitude) == true)
		{
			g_APRS_frequency = 144800000;
			g_APRS_no_tx = true;
		}
		else if(point_in_polygon(11, lat, latitude, longitude) == true)
		{
			g_APRS_frequency = 144800000;
			g_APRS_no_tx = true;
		}
		else if(point_in_polygon(5, yem, latitude, longitude) == true)
		{
			g_APRS_frequency = 144800000;
			g_APRS_no_tx = true;
		}
		else if(point_in_polygon(8, bra, latitude, longitude) == true)
		{
			g_APRS_frequency = 144570000;
			g_APRS_no_tx = false;
		}
		else
		{
			g_APRS_frequency = 144800000;
			g_APRS_no_tx = false;
		}
	}
	//america
	else if(longitude <= -40.0)
	{
		if(latitude > 12.5)
		{
			g_APRS_frequency = 144390000;
			g_APRS_no_tx = false;
		}
		else
		{
			if(point_in_polygon(13, arg_par_uru, latitude, longitude) == true)
			{
				g_APRS_frequency = 144930000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(8, bra, latitude, longitude) == true)
			{
				g_APRS_frequency = 144570000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(10, ven, latitude, longitude) == true)
			{
				g_APRS_frequency = 145010000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, cos_nic_pan, latitude, longitude) == true)
			{
				g_APRS_frequency = 145010000;
				g_APRS_no_tx = false;
			}
			else
			{
				g_APRS_frequency = 144390000;
				g_APRS_no_tx = false;
			}
		}
	}
	else if(longitude >= 70.0)
	{
		if(latitude > 19)
		{
			if(point_in_polygon(7, nor_kor, latitude, longitude) == true)
			{
				g_APRS_frequency = 144800000;
				g_APRS_no_tx = true;
			}
			else if(point_in_polygon(15, chi, latitude, longitude) == true)
			{
				g_APRS_frequency = 144640000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(7, jap, latitude, longitude) == true)
			{
				g_APRS_frequency = 144660000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, sou_kor, latitude, longitude) == true)
			{
				g_APRS_frequency = 144620000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, tha, latitude, longitude) == true)
			{
				g_APRS_frequency = 144525000;
				g_APRS_no_tx = false;
			}
			else
			{
				g_APRS_frequency = 144800000;
				g_APRS_no_tx = false;
			}
		}
		else
		{
			if(point_in_polygon(6, aus, latitude, longitude) == true)
			{
				g_APRS_frequency = 144175000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, new_zel, latitude, longitude) == true)
			{
				g_APRS_frequency = 144575000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, sou_kor, latitude, longitude) == true)
			{
				g_APRS_frequency = 144620000;
				g_APRS_no_tx = false;
			}
			else if(point_in_polygon(5, tha, latitude, longitude) == true)
			{
				g_APRS_frequency = 144525000;
				g_APRS_no_tx = false;
			}
			else
			{
				g_APRS_frequency = 144390000;
				g_APRS_no_tx = false;
			}
		}
	}
	else
	{
		g_APRS_frequency = 144800000;
		g_APRS_no_tx = false;
	}
}
