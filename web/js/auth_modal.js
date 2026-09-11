/**
 * ContextLab — Embrapa OTP Authentication Module
 */

import { api } from '../api.js';
import { state } from '../state.js';

export class AuthManager {
  constructor(options = {}) {
    this.modal = document.getElementById('auth-otp-modal');
    this.btnTrigger = document.getElementById('btn-header-login');
    this.userBadge = document.getElementById('user-profile-badge');
    this.onLoginCallback = options.onLogin || (() => {});
    this.currentEmail = '';
    this.init();
  }

  async init() {
    this.bindEvents();
    await this.checkExistingSession();
  }

  async checkExistingSession() {
    try {
      const res = await api.getMe();
      if (res.authenticated && res.user) {
        state.setUser(res.user);
        this.renderUserBadge(res.user);
      } else {
        state.setUser(null);
        this.renderLoggedOut();
      }
    } catch {
      this.renderLoggedOut();
    }
  }

  bindEvents() {
    if (this.btnTrigger) {
      this.btnTrigger.addEventListener('click', () => this.openModal());
    }

    const btnClose = document.getElementById('btn-close-auth-modal');
    if (btnClose) {
      btnClose.addEventListener('click', () => this.closeModal());
    }

    // Step 1: Request OTP Form
    const formRequest = document.getElementById('form-request-otp');
    if (formRequest) {
      formRequest.addEventListener('submit', async (e) => {
        e.preventDefault();
        const emailInput = document.getElementById('auth-email-input');
        const email = emailInput ? emailInput.value.trim() : '';
        await this.handleRequestOtp(email);
      });
    }

    // Step 2: Verify OTP Form
    const formVerify = document.getElementById('form-verify-otp');
    if (formVerify) {
      formVerify.addEventListener('submit', async (e) => {
        e.preventDefault();
        const code = this.getOtpValue();
        await this.handleVerifyOtp(this.currentEmail, code);
      });
    }

    // OTP inputs auto-advance
    const otpInputs = document.querySelectorAll('.cl-otp-digit');
    otpInputs.forEach((input, index) => {
      input.addEventListener('input', (e) => {
        if (e.target.value.length === 1 && index < otpInputs.length - 1) {
          otpInputs[index + 1].focus();
        }
      });
      input.addEventListener('keydown', (e) => {
        if (e.key === 'Backspace' && !e.target.value && index > 0) {
          otpInputs[index - 1].focus();
        }
      });
    });

    // Dev auto-fill button
    const btnAutofill = document.getElementById('btn-dev-autofill');
    if (btnAutofill) {
      btnAutofill.addEventListener('click', () => {
        const code = btnAutofill.dataset.code || '';
        this.setOtpValue(code);
      });
    }

    // Back to Step 1
    const btnBack = document.getElementById('btn-auth-back');
    if (btnBack) {
      btnBack.addEventListener('click', () => this.showStep(1));
    }

    // Logout button
    const btnLogout = document.getElementById('btn-header-logout');
    if (btnLogout) {
      btnLogout.addEventListener('click', async () => {
        await api.logout();
        state.setUser(null);
        this.renderLoggedOut();
        this.onLoginCallback(null);
      });
    }
  }

  openModal() {
    if (!this.modal) return;
    this.showStep(1);
    this.modal.classList.add('active');
    const emailInput = document.getElementById('auth-email-input');
    if (emailInput) emailInput.focus();
  }

  closeModal() {
    if (!this.modal) return;
    this.modal.classList.remove('active');
    this.clearAlerts();
  }

  showStep(stepNumber) {
    const step1 = document.getElementById('auth-step-1');
    const step2 = document.getElementById('auth-step-2');
    if (step1 && step2) {
      step1.style.display = stepNumber === 1 ? 'block' : 'none';
      step2.style.display = stepNumber === 2 ? 'block' : 'none';
    }
    this.clearAlerts();
  }

  async handleRequestOtp(email) {
    const alertEl = document.getElementById('auth-request-alert');
    if (!email.toLowerCase().endsWith('@embrapa.br')) {
      this.showAlert(alertEl, 'Por favor, informe um endereço válido do domínio @embrapa.br', 'error');
      return;
    }

    try {
      this.showAlert(alertEl, 'Enviando código...', 'info');
      const res = await api.requestOtp(email);
      this.currentEmail = email;

      // Update step 2 UI
      const emailDisplay = document.getElementById('auth-verify-email-display');
      if (emailDisplay) emailDisplay.textContent = email;

      const devBox = document.getElementById('auth-dev-hint');
      const btnAutofill = document.getElementById('btn-dev-autofill');
      if (res.dev_otp && devBox && btnAutofill) {
        devBox.style.display = 'flex';
        btnAutofill.dataset.code = res.dev_otp;
        const codeSpan = document.getElementById('auth-dev-code-val');
        if (codeSpan) codeSpan.textContent = res.dev_otp;
      }

      this.showStep(2);
      const firstDigit = document.querySelector('.cl-otp-digit');
      if (firstDigit) firstDigit.focus();
    } catch (err) {
      this.showAlert(alertEl, err.message, 'error');
    }
  }

  async handleVerifyOtp(email, code) {
    const alertEl = document.getElementById('auth-verify-alert');
    if (code.length !== 6) {
      this.showAlert(alertEl, 'Digite o código de 6 dígitos completo', 'error');
      return;
    }

    try {
      this.showAlert(alertEl, 'Verificando código...', 'info');
      const res = await api.verifyOtp(email, code);
      state.setUser(res.user);
      this.renderUserBadge(res.user);
      this.closeModal();
      this.onLoginCallback(res.user);
    } catch (err) {
      this.showAlert(alertEl, err.message, 'error');
    }
  }

  getOtpValue() {
    const digits = Array.from(document.querySelectorAll('.cl-otp-digit')).map(i => i.value);
    return digits.join('');
  }

  setOtpValue(code) {
    const inputs = document.querySelectorAll('.cl-otp-digit');
    for (let i = 0; i < inputs.length; i++) {
      inputs[i].value = code[i] || '';
    }
  }

  showAlert(el, message, type = 'info') {
    if (!el) return;
    el.textContent = message;
    el.style.display = 'block';
    el.style.color = type === 'error' ? '#f43f5e' : (type === 'success' ? '#28dea0' : '#3898ff');
  }

  clearAlerts() {
    ['auth-request-alert', 'auth-verify-alert'].forEach(id => {
      const el = document.getElementById(id);
      if (el) { el.textContent = ''; el.style.display = 'none'; }
    });
  }

  renderUserBadge(user) {
    if (this.btnTrigger) this.btnTrigger.style.display = 'none';
    if (this.userBadge) {
      this.userBadge.style.display = 'flex';
      const initials = user.name ? user.name.split(' ').map(n => n[0]).slice(0, 2).join('') : 'EM';
      const avatar = document.getElementById('user-badge-avatar');
      const name = document.getElementById('user-badge-name');
      const unit = document.getElementById('user-badge-unit');
      if (avatar) avatar.textContent = initials.toUpperCase();
      if (name) name.textContent = user.name;
      if (unit) unit.textContent = user.unit || 'Embrapa';
    }
  }

  renderLoggedOut() {
    if (this.btnTrigger) this.btnTrigger.style.display = 'inline-flex';
    if (this.userBadge) this.userBadge.style.display = 'none';
  }
}
