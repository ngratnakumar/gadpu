from flask import Flask
from flask_restful import Api, Resource, reqparse

app = Flask(__name__)
api = Api(app)

users = [
    {
        "name": "Ratna kumar",
        "age": 26,
        "occupation": "Project Engineer"
    }
]

class User(Resource):
    def get(self, name):

    def post(self, name):

    def put(self, name):

    def delete(self, name):

