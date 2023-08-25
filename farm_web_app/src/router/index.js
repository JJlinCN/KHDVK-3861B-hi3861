import { createRouter, createWebHistory } from 'vue-router'

const home = () => import("../components/home.vue")
const login = () => import("../components/login.vue")
const register = () => import("../components/register.vue")

const routes = [
  {
    path: '/',
    redirect: '/login'
  },
  {
    path: '/login',
    name: 'login',
    component: login
  },
  {
    path: '/register',
    name: 'register',
    component: register
  },
  {
    path: '/home',
    name: 'home',
    component: home,
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router
