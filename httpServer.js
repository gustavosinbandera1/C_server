
var express = require('express');
var bodyParser = require('body-parser');
var http = require('http');
const path = require('path');

var app = express();
app.use( bodyParser.json() );       // to support JSON-encoded bodies
app.use(bodyParser.urlencoded({     // to support URL-encoded bodies
  extended: true
}));



app.post('/', function (req, res) {
  //res.send('Hello World!');
  console.log(JSON.stringify(req.body));
  res.json({requestBody: req.body}) 
  res.end();
});

app.listen(4000, function () {
  console.log('Example app listening on port 4000!');
});
