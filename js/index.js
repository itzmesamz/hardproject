var express = require('express')
var bodyParser = require('body-parser')

var app = express()

app.use(bodyParser.raw())

app.post('/', (req, res) => {
    const buffer = req.body
    // const arr = []
    // for (let i=0; i<buffer.length; i+=2) {
    //     arr.push(buffer.readUInt16LE(i))
    // }
    // console.log(arr)
    res.send(buffer.length.toString())
    console.log(buffer.length)
})

app.listen(3000, () => {
    console.log('mulai di port 3000')
})