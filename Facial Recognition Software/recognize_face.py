#Import libraries
print('Importing libraries')
import face_recognition
import boto3
import base64
from PIL import Image
import io
import time
import numpy as np
import json

print('Starting Lambda')
BUCKET_NAME = '******'                                                  # Fake name
s3 = boto3.client('s3')

def lambda_handler(event, context):
    try:
        # Start the Lambda handler
        print('Starting Lambda handler')
        headers = event.get('headers')

        # Authorize the request
        print('Authorizing request')
        token = headers.get('device-token')
        if not token or token != '*****':                               # Fake token
            raise Exception('Unauthorized request')

        # Gather the authorized faces in the S3 bucket
        print('Gathering known images')
        known_encodings = []
        keys = []

        paginator = s3.get_paginator('list_objects_v2')
        for page in paginator.paginate(
            Bucket = BUCKET_NAME,
            Prefix = 'known_images/'
        ):
            for obj in page.get('Contents', []):
                key = obj.get('Key')
                if not key.endswith('.json'):
                    continue

                keys.append(key)

                obj_body = s3.get_object(
                    Bucket = BUCKET_NAME,
                    Key=key
                ).get('Body')

                obj_body_bytes = obj_body.read()
                data = json.loads(obj_body_bytes.decode('utf-8'))

                encoding = np.array(data.get('encoding'), dtype = np.float64)
                known_encodings.append(encoding)

        # Gather the image sent by the request
        print('Gathering given image')
        body = event.get('body')
        if event.get('isBase64Encoded'):
            image_bytes = base64.b64decode(body)
            given_image = np.array(Image.open(io.BytesIO(image_bytes)).convert('RGB'))
        else:
            raise Exception('Invalid request')

        # Process the image sent by the request
        print('Processing given image')
        given_image_encodings = face_recognition.face_encodings(given_image)

        if len(given_image_encodings) == 0:
            raise Exception('No face found in the given image')

        given_image_encoding = given_image_encodings[0]

        # Compare the image sent by the request to the images in the S3 bucket
        print('Comparing images')
        matches = face_recognition.compare_faces(known_encodings, given_image_encoding, tolerance = 0.6)
        
        # Perform the given operation
        print('Performing given operation')
        operation = headers.get('operation')

        if not operation:
            raise Exception('No operation given')

        if operation == 'upload' or operation == 'delete':
            # Delete all matching images
            for i in range(len(matches)):
                if matches[i]:
                    s3.delete_object(
                        Bucket = BUCKET_NAME,
                        Key = keys[i]
                    )

            if operation == 'upload':
                # Save the image sent by the request
                key = f'known_images/{int(time.time())}.json'
                s3.put_object(
                    Bucket=BUCKET_NAME,
                    Key=key,
                    Body=json.dumps({'encoding': given_image_encoding.tolist()}).encode('utf-8'),
                    ContentType='application/json',
                )

        elif operation == 'validate':
            # Check if there is a match
            if not any(matches):
                raise Exception('No match found')

        return {
            'statusCode': 200,
            'headers': {'content-type': 'application/json'},
            'body': json.dumps({'message': 'Operation successful'}),
        }

    except Exception as e:
        return {
            'statusCode': 500,
            'headers': {'content-type': 'application/json'},
            'body': json.dumps({'error': str(e)}),
        }
