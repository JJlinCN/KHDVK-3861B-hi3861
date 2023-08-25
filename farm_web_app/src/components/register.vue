<template>
    <div>
      <video autoplay loop muted class="bg-video">
      <source type="video/mp4" src="https://career.djicdn.com/broadway/public/9f013c2d-a933-4202-8ac3-d68d4ac160c8/index-banner.mp4">
      </video>
      <div id="register" class="register-container">
        <h2>智慧农业灌溉系统 | 注册</h2>
        <el-form :model="form" label-width="auto">
          <el-form-item label="用户名:">
            <el-input v-model="form.name"></el-input>
          </el-form-item>
          <el-form-item label="密码:">
            <el-input v-model="form.password" type="password"></el-input>
          </el-form-item>
        </el-form>
        <el-button type="primary" round @click="register" class="btn">注册</el-button>
      </div>
    </div>
  </template>
  
  <script setup>
  import { reactive } from 'vue';
  import axios from 'axios';
  import { ElMessage, ElMessageBox } from 'element-plus';
  import { useRouter } from 'vue-router'
  const form = reactive({
    name: '',
    password: ''
  });
  
  const router = useRouter();

  const register = () => {
    if (form.name === '') {
      ElMessage.error('用户名不能为空');
    } else if (form.password === '') {
      ElMessage.error('密码不能为空');
    } else {
      axios
        .post('http://127.0.0.1/register', form)
        .then(res => {
          if (res.data.status === 200) {
            ElMessageBox.alert('是否返回登录页面', '注册成功', {
              confirmButtonText: '确定',
              callback: action => {
                router.push('/login');
              }
            });
          } else if (res.data.status === 202) {
            ElMessageBox.alert('用户名已存在', '注册失败', {
              confirmButtonText: '确定',
              callback: action => {
                form.name = '';
                form.password = '';
              }
            });
          } else {
            console.log(res.message);
          }
        })
        .catch(err => {
          console.log('操作失败' + err);
        });
    }
  };
  </script>
  
  <style>
  .bg {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: -1;
    object-fit: cover;
  }
  
  .register-container {
    /* 调整登录容器的样式 */
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    background-color: white;
    padding: 24px;
  }
  </style>
  