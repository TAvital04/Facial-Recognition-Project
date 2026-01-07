print('Importing libraries')
import face_recognition
import boto3
import os

print('Starting lambda')

def terminate_folder():
    print('Terminating project folder')

    if os.access('./Images/Known', os.F_OK):
        for dir in os.listdir('Images/Known'):
            os.remove(f'./Images/Known/{dir}')
        os.rmdir('Images/Known')

    if os.access('./Images/Unknown', os.F_OK):
        for dir in os.listdir('Images/Unknown'):
            os.remove(f'./Images/Unknown/{dir}')
        os.rmdir('Images/Unknown')

    os.rmdir('Images')

def lambda_handler():
    try:
        print('Starting lambda handler')

        print('Setting up project folder')
        if os.access('./Images', os.F_OK):
            terminate_folder()

        os.mkdir('Images')
        os.mkdir('Images/Known')
        os.mkdir('Images/Unknown')

        print('Gathering known images')
        s3_client = boto3.client('s3')
        s3_resource = boto3.resource('s3')

        bucket_name = 'talav-facial-recognition-project'
        bucket = s3_resource.Bucket(bucket_name)

        known_images = []
        for object in bucket.objects.filter(Prefix='known_images/'):
            filename = object.key.removeprefix('known_images/')

            if len(filename) == 0:
                continue

            path = f'./Images/Known/{filename}'
            open(path, 'a')

            s3_client.download_file(Bucket = bucket_name, Key = object.key, Filename = path)
            known_images.append(face_recognition.load_image_file(path))

        print('Processing known images')
        known_images_encoded = []
        for image in known_images:
            encoded_image = face_recognition.face_encodings(image)
            known_images_encoded.append(encoded_image)

        print('Gathering given image')
        unknown_image = face_recognition.load_image_file('./WIN_20251227_21_37_51_Pro.jpg')

        print('Processing given image')
        unknown_image_encoded = face_recognition.face_encodings(unknown_image)[0]

        print('Comparing images')
        result = False
        for image in known_images_encoded:
            comparison = face_recognition.compare_faces(image, unknown_image_encoded, .6)

            if comparison[0]:
                result = True
                continue

        print(f'Result: {result}')

        terminate_folder()

        # print('Returning results')
        # return {
        #     'statusCode': 200,
        #     'body': json.dumps('Hello from Lambda!')
        # }

    except Exception as e:
        print(f'Error: {e}')
        terminate_folder()
        # return {
        #     'statusCode': 400,
        #     'body': json.dumps(e)
        # }

lambda_handler()