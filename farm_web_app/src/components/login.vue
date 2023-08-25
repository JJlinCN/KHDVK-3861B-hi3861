<template>
  <div>
    <video autoplay loop muted class="bg-video">
      <source type="video/mp4" src="https://career.djicdn.com/broadway/public/8911c6f1-9651-4451-8fca-7a9953b20665/campus-index-pc.mp4">
    </video>

    <div id="login" class="login-container">
      <!-- 剩下的登录界面内容 -->
      <h2>智慧农业灌溉系统 | 登录</h2>
          <el-form :model="form" label-width="auto">
          <el-form-item label="用户名:">
              <el-input v-model="form.username"></el-input>
          </el-form-item>
          <el-form-item label="密  码:">
              <el-input v-model="form.password" type="password"></el-input>
          </el-form-item>
          </el-form>
          <el-button type="primary" round @click="login" class="btn">登录</el-button>
          <el-button round @click="register">注册</el-button>
    </div>
  </div>
</template>

  
  <script setup>
  import { reactive, ref } from 'vue';
  import axios from 'axios';
  import { ElMessage, ElMessageBox } from 'element-plus';
  import { useRouter } from 'vue-router'
  const form = reactive({
    username: '',
    password: ''
  });
  const router = useRouter();
  const login = () => {
    if (form.username === '') {
      ElMessage.error('用户名不能为空');
    } else if (form.password === '') {
      ElMessage.error('密码不能为空');
    } else {
      axios
        .get('http://127.0.0.1/login', {
          params: {
            name: form.username,
            password: form.password
          }
        })
        .then(res => {
          if (res.data.status === 200) {
            router.push({
              path: '/home',
              query: {
                name: form.username
              }
            });
          } else {
            ElMessageBox.alert('用户名或密码错误','登录失败',{
              confirmButtonText: '确定',
              callback: action => {
                form.username = '';
                form.password = '';
              }
            });
          }
        })
        .catch(err => {
          console.log('登录失败' + err);
        });
    }
  };
  
  const register = () => {
    router.push('/register');
  };
  </script>
  
  
  <style>
  .bg-video {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: -1;
    object-fit: cover;
  }

  .login-container {
    /* 调整登录容器的样式 */
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    background-color: white;
    padding: 24px;
  }
</style>
