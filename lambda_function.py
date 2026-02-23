"""AWS Lambda Function Handler for Aerio Camera Image Upload

This Lambda function acts as the "Brain" for the Aerio camera system.
It receives image data from the ESP32 via HTTP POST and uploads it to S3.

Configuration:
- Function URL: Public, Auth type: NONE, CORS: Enabled
- IAM Role: camera-upload-handler-role with S3FullAccess policy
- S3 Bucket: aerio-camera-storage-2026

Note: The closing brace on line 24 is CRITICAL - missing it causes syntax errors!
"""

import json
import base64
import boto3
import time

s3 = boto3.client('s3')
BUCKET_NAME = 'aerio-camera-storage-2026'  # Update with your bucket name

def lambda_handler(event, context):
    """Main Lambda handler function
    
    Args:
        event: API Gateway/Function URL event containing binary image data
        context: Lambda context object
        
    Returns:
        dict: Response with statusCode and body (JSON)
    """
    try:
        # Decode binary data from the Function URL
        if event.get('isBase64Encoded'):
            file_content = base64.b64decode(event['body'])
        else:
            file_content = event['body'].encode('utf-8') if isinstance(event['body'], str) else event['body']

        # Generate timestamped filename
        file_name = f"IMG_{int(time.time())}.jpg"
        
        # Upload to S3
        s3.put_object(Bucket=BUCKET_NAME, Key=file_name, Body=file_content, ContentType='image/jpeg')

        return {
            'statusCode': 200,
            'body': json.dumps({'status': 'Success', 'file': file_name, 'size': len(file_content)})
        }
    except Exception as e:
        return {'statusCode': 500, 'body': json.dumps({'error': str(e)})}
