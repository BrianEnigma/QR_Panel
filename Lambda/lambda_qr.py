#!/usr/bin/env python3

from __future__ import print_function # Python 2/3 compatibility
import boto3
import json
import random


Topic = 'qr_trigger'


def getRandomColors():
    """
    Get two _DIFFERENT_ random colors from a small palette of easily-identifiable colors.
    :return:
    """
    Colors = [
        [[0x00, 0x00, 0x00], 'black'],
        [[0xff, 0x00, 0x00], 'red'],
        [[0x00, 0xff, 0x00], 'green'],
        [[0x00, 0x00, 0xff], 'blue'],
        [[0xff, 0xff, 0x00], 'yellow'],
        [[0xff, 0x00, 0xff], 'magenta'],
        [[0x00, 0xff, 0xff], 'cyan'],
        [[0xff, 0xff, 0xff], 'white']
    ]
    colors = random.sample(Colors, 2)
    return colors


def get_cors(event):
    referer = None
    result = 'https://3random.org'

    # Grab the Referer header, if it exists.
    try:
        referer = event['headers']['Referer']
    except:
        # referer = None
        referer = ''

    # If it doesn't exist at all, return the default.
    if referer is None:
        return result

    # If it exists and is empty, return all.
    if len(referer) == 0 or referer == 'null':
        return '*'

    # If it exists and has a 'www' then return the 'www' version.
    try:
        if referer.index('qr') >= 0:
            result = 'https://qr.netninja.com'
    except:
        pass

    return result


def lambda_handler(event, context):
    mqtt = boto3.client('iot-data', region_name='us-west-2')
    # TODO: Randomize based on IP address
    colors = getRandomColors()
    # TODO: Allow Override
    doc = {'color0': colors[0][0], 'color1': colors[1][0]}
    payload = json.dumps(doc)
    mqtt.publish(topic=Topic, payload=payload)
    response_body = {
        'colors': [colors[0][0], colors[1][0]],
        'color_names': [colors[0][1], colors[1][1]]
    }
    result = {
        'statusCode': 200,
        'body': json.dumps(response_body),
        'headers': {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': get_cors(event)
        }
    }
    return result
