let express = require('express')
let router = express.Router()
let user= require('./API/user')
let info = require('./API/info')
let login = require('./API/login')

router.get('/user', user.get)
router.get('/info/all', info.all)
router.get('/info/get', info.get)
router.get('/info/add', info.add)
router.get('/info/update', info.update)
router.get('/info/del', info.del)
router.get('/login', login.login)
router.post('/register', login.register)

module.exports = router

